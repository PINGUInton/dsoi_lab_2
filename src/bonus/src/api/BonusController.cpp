#include "BonusController.hpp"
#include <sstream>

// Health check endpoint
crow::response BonusController::health_check() {
    try {
        if (!bonus_repository.is_connected()) {
            return create_error_response(500, "Database not connected");
        }

        nlohmann::json response = {
            {"status", "OK"}
        };

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        return crow::response(500);
    }
}

// GET /api/v1/privilege - получить информацию о бонусном счете
crow::response BonusController::get_privilege_info(const crow::request& req) {
    try {
        std::string username = req.get_header_value("X-User-Name");

        if (username.empty()) {
            return create_error_response(400, "Username header is required");
        }

        auto privilege_opt = bonus_repository.get_privilege_by_username(username);

        nlohmann::json response;

        if (!privilege_opt) {
            Privilege new_privilege = bonus_repository.create_privilege(username, 0);
            response = new_privilege.to_api_json();
        }
        else {
            Privilege privilege = privilege_opt.value();

            auto history = bonus_repository.get_privilege_history(privilege.id);

            response = privilege.to_api_json();
            response["history"] = nlohmann::json::array();

            for (const auto& history_item : history) {
                response["history"].push_back(history_item.to_json());
            }
        }

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Error getting privilege info: " << e.what() << std::endl;
        return create_error_response(500, "Internal server error");
    }
}

// Обновление баланса привилегий
crow::response BonusController::update_privilege_balance(const crow::request& req,
    const std::string& username,
    const std::string& ticket_uid,
    int balance_diff,
    const std::string& operation_type) {
    try {
        if (username.empty()) {
            return create_error_response(400, "Username is required");
        }

        bool success = bonus_repository.update_privilege_balance(
            username, ticket_uid, balance_diff, operation_type
        );

        if (!success) {
            return create_error_response(500, "Failed to update privilege balance");
        }

        nlohmann::json response = {
            {"status", "success"},
            {"message", "Balance updated successfully"}
        };

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Error updating privilege balance: " << e.what() << std::endl;
        return create_error_response(500, "Internal server error");
    }
}

// Создание ошибки
crow::response BonusController::create_error_response(int status_code, const std::string& message) {
    nlohmann::json error = {
        {"message", message}
    };

    crow::response res(status_code, error.dump());
    res.set_header("Content-Type", "application/json");
    return res;
}

// Роутер
void BonusController::router(crow::SimpleApp& app) {
    // Health check
    CROW_ROUTE(app, "/manage/health")
        .methods("GET"_method)
        ([this]() {
        return this->health_check();
            });

    // GET /api/v1/privilege - информация о бонусном счете
    CROW_ROUTE(app, "/api/v1/privilege")
        .methods("GET"_method)
        ([this](const crow::request& req) {
        return this->get_privilege_info(req);
            });

    // POST /api/v1/privilege/update - обновление баланса (внутренний endpoint)
    CROW_ROUTE(app, "/api/v1/privilege/update")
        .methods("POST"_method)
        ([this](const crow::request& req) {
        try {
            auto json_body = nlohmann::json::parse(req.body);

            std::string username = json_body["username"];
            std::string ticket_uid = json_body["ticketUid"];
            int balance_diff = json_body["balanceDiff"];
            std::string operation_type = json_body["operationType"];

            return this->update_privilege_balance(req, username, ticket_uid,
                balance_diff, operation_type);
        }
        catch (const std::exception& e) {
            return create_error_response(400, "Invalid request body");
        }
            });
}