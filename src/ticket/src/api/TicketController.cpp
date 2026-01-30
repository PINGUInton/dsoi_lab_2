#include "TicketController.hpp"
#include <sstream>
#include <algorithm>

// Health check endpoint
crow::response TicketController::health_check() {
    try {
        if (!ticket_repository.is_connected()) {
            return create_error_response(500, "Database not connected");
        }
        
        nlohmann::json response = {
            {"status", "OK"}
        };
        
        crow::response res(200, response.dump());
        
        res.set_header("Content-Type", "application/json");
        return res;
        
    } catch (const std::exception& e) {
        return crow::response(500);
    }
}

// Создание массива ошибок
crow::response TicketController::create_error_array_response(const std::string err_type = "both") {
    
    nlohmann::json response = {
        {"message", "Ошибка валидации данных"},
        {"errors", nlohmann::json::array()}
    };

    nlohmann::json field_price = {
        {"field", "price"},
        {"description", "Стоимость должна быть положительным числом"}
    };

    nlohmann::json field_flightNumber = {
        {"field", "flightNumber"},
        {"description", "Номер рейса обязателен для заполнения"}
    };

    if (err_type == "both") {
        response["errors"].push_back(field_price);
        response["errors"].push_back(field_flightNumber);
    }
    else if (err_type == "price") {
        response["errors"].push_back(field_price);
    }
    else if (err_type == "flightNumber") {
        response["errors"].push_back(field_flightNumber);
    }
    
    crow::response res(400, response.dump());

    res.set_header("Content-Type", "application/json");
    return res;
}

// Создание ошибки
crow::response TicketController::create_error_response(int status_code, const std::string& message) {
    nlohmann::json error = {
        {"message", message}
    };

    crow::response res(status_code, error.dump());

    res.set_header("Content-Type", "application/json");
    return res;
}

crow::response TicketController::validate_ticket_creation(const nlohmann::json& json_body) {
    if (json_body.empty()) {
        return create_error_array_response();
    }

    if (!json_body.contains("price")) {
        return create_error_array_response("price");
    }

    if (!json_body.contains("flightNumber")) {
        return create_error_array_response("flightNumber");
    }

    if (!json_body["price"].is_number()) {
        return create_error_array_response("price");
    }
    
    int price = json_body["price"];

    if (price <= 0) {
        return create_error_array_response("price");
    }
    
    return crow::response(200);
}

// GET /api/v1/tickets - получить все билеты пользователя
crow::response TicketController::get_user_tickets(const crow::request& req) {
    try {
        std::string username = req.get_header_value("X-User-Name");
        
        if (username.empty()) {
            return create_error_response(404, "Билеты не найдены");
        }
        
        auto tickets = ticket_repository.get_tickets_by_username(username);
        
        nlohmann::json response = nlohmann::json::array();
        
        for (const auto& ticket : tickets) {
            response.push_back(ticket.to_api_json());
        }
        
        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
        
    } catch (const std::exception& e) {
        return crow::response(500);
    }
}

// GET /api/v1/tickets/{ticketUid} - получить конкретный билет
crow::response TicketController::get_ticket_by_uid(const crow::request& req, const std::string& ticket_uid) {
    try {
        std::string username = req.get_header_value("X-User-Name");
        
        
        
        
        if (username.empty()) {
            return create_error_response(404, "Билет не найден");
        }
        
        if (ticket_uid.length() != 36) {
            return create_error_response(404, "Билет не найден");
        }



        auto ticket_opt = ticket_repository.get_ticket_by_uid(ticket_uid);
        

        if (!ticket_opt) {
            return create_error_response(404, "Билет не найден");
        }
        
        Ticket ticket = ticket_opt.value();
        
        if (ticket.username != username) {
            return create_error_response(404, "Билет не найден");
        }


        crow::response res(200, ticket.to_api_json().dump());


        res.set_header("Content-Type", "application/json");
        
        return res;
    } catch (const std::exception& e) {
        return crow::response(500);
    }
}

// POST /api/v1/tickets - покупка билета (в контексте сервиса его создание)
crow::response TicketController::create_ticket(const crow::request& req) {
    try {
        std::string username = req.get_header_value("X-User-Name");

        crow::response res(201);

        nlohmann::json json_body;
        json_body = nlohmann::json::parse(req.body);

        res = validate_ticket_creation(json_body);
        
        if (res.code != 200) {
            return res;
        }

        int price = json_body["price"];
        std::string flight_number = json_body["flightNumber"];
        std::string status = "PAID";
        
        Ticket ticket = ticket_repository.create_ticket(username, flight_number, price, status);
        nlohmann::json response = ticket.to_api_json();
        
        res = crow::response(200, response.dump());

        res.set_header("Content-Type", "application/json");
        return res;
        
    } catch (const std::exception& e) {
        return crow::response(500);
    }
}

// DELETE /api/v1/tickets/{ticketUid} - возврат
crow::response TicketController::canceled_ticket(const crow::request& req, const std::string& ticket_uid) {
    try {
        std::string username = req.get_header_value("X-User-Name");
        if (username.empty()) {
            return create_error_response(404, "Билет не найден");
        }

        if (!ticket_repository.ticket_belongs_to_user(ticket_uid, username)) {
            return create_error_response(404, "Билет не найден");
        }
        
        bool updated = ticket_repository.update_ticket_status(ticket_uid, "CANCELED");
        
        if (!updated) {
            return create_error_response(404, "Билет не найден");
        }
        
        return crow::response(204);
        
    } catch (const std::exception& e) {
        return crow::response(500);
    }
}

// Роутер
void TicketController::router(crow::SimpleApp& app) {
    // health check
    CROW_ROUTE(app, "/manage/health")
        .methods("GET"_method)
    ([this]() {
        return this->health_check();
    });
    
    // GET /api/v1/tickets - все билеты пользователя
    CROW_ROUTE(app, "/api/v1/tickets")
        .methods("GET"_method)
    ([this](const crow::request& req) {
        return this->get_user_tickets(req);
    });
    
    // POST /api/v1/tickets - создать билет
    CROW_ROUTE(app, "/api/v1/tickets")
        .methods("POST"_method)
    ([this](const crow::request& req) {
        return this->create_ticket(req);
    });
    
    // GET /api/v1/tickets/{ticketUid} - получить конкретный билет
    CROW_ROUTE(app, "/api/v1/tickets/<string>")
        .methods("GET"_method)
    ([this](const crow::request& req, const std::string& ticket_uid) {
        return this->get_ticket_by_uid(req, ticket_uid);
    });
    
    // DELETE /api/v1/tickets/{ticketUid} - возврат
    CROW_ROUTE(app, "/api/v1/tickets/<string>")
        .methods("DELETE"_method)
    ([this](const crow::request& req, const std::string& ticket_uid) {
        return this->canceled_ticket(req, ticket_uid);
    });
}