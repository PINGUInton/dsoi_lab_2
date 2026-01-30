#ifdef _MSC_VER
#define _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
#endif

#include "GatewayController.hpp"
#include <sstream>
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace gateway_utils {
    // В Linux utility::string_t - это std::string, в Windows - std::wstring
    inline std::string to_utf8string(const utility::string_t& str) {
#ifdef _WIN32
        return utility::conversions::to_utf8string(str);
#else
        return str; // В Linux уже UTF-8
#endif
    }

    inline utility::string_t to_string_t(const std::string& str) {
#ifdef _WIN32
        return utility::conversions::to_string_t(str);
#else
        return str;
#endif
    }

    // Вспомогательная функция для доступа к строковым полям JSON
    inline std::string get_json_string_field(const web::json::value& json,
        const std::string& field,
        const std::string& default_value = "") {
        if (json.is_null() || !json.has_string_field(to_string_t(field))) {
            return default_value;
        }
        try {
            return to_utf8string(json.at(to_string_t(field)).as_string());
        }
        catch (...) {
            return default_value;
        }
    }

    // Вспомогательная функция для доступа к числовым полям JSON
    inline int get_json_int_field(const web::json::value& json,
        const std::string& field,
        int default_value = 0) {
        if (json.is_null() || !json.has_integer_field(to_string_t(field))) {
            return default_value;
        }
        try {
            return json.at(to_string_t(field)).as_integer();
        }
        catch (...) {
            return default_value;
        }
    }

    // Вспомогательная функция для доступа к булевым полям JSON
    inline bool get_json_bool_field(const web::json::value& json,
        const std::string& field,
        bool default_value = false) {
        if (json.is_null() || !json.has_boolean_field(to_string_t(field))) {
            return default_value;
        }
        try {
            return json.at(to_string_t(field)).as_bool();
        }
        catch (...) {
            return default_value;
        }
    }

    // Вспомогательная функция для получения массива как web::json::value
    inline web::json::value get_json_array_as_value(const web::json::value& json,
        const std::string& field) {
        if (json.is_null() || !json.has_field(to_string_t(field))) {
            return web::json::value::array();
        }
        try {
            return json.at(to_string_t(field));
        }
        catch (...) {
            return web::json::value::array();
        }
    }
}

using gateway_utils::to_utf8string;
using gateway_utils::to_string_t;
using gateway_utils::get_json_string_field;
using gateway_utils::get_json_int_field;
using gateway_utils::get_json_bool_field;
using gateway_utils::get_json_array_as_value;

// Health check
crow::response GatewayController::health_check() {
    try {
        bool all_healthy = true;
        std::vector<std::string> failed_services;

        try {
            auto flight_response = call_service_sync(flight_service_url + "/manage/health", methods::GET);
        }
        catch (const std::exception& e) {
            all_healthy = false;
            failed_services.push_back("Flight Service");
        }

        try {
            auto ticket_response = call_service_sync(ticket_service_url + "/manage/health", methods::GET);
        }
        catch (const std::exception& e) {
            all_healthy = false;
            failed_services.push_back("Ticket Service");
        }

        try {
            auto bonus_response = call_service_sync(bonus_service_url + "/manage/health", methods::GET);
        }
        catch (const std::exception& e) {
            all_healthy = false;
            failed_services.push_back("Bonus Service");
        }

        nlohmann::json response = {
            {"status", all_healthy ? "OK" : "DEGRADED"},
            {"services", {
                {"flight_service", flight_service_url},
                {"ticket_service", ticket_service_url},
                {"bonus_service", bonus_service_url}
            }}
        };

        if (!all_healthy) {
            response["failed_services"] = failed_services;
        }

        crow::response res(all_healthy ? 200 : 503, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Health check error: " << e.what() << std::endl;
        return create_error_response(500, "Internal server error");
    }
}

// Создание ошибки
crow::response GatewayController::create_error_response(int status_code, const std::string& message) {
    nlohmann::json error = {
        {"message", message}
    };

    crow::response res(status_code, error.dump());
    res.set_header("Content-Type", "application/json");
    return res;
}

// Валидация запроса на покупку
crow::response GatewayController::validate_purchase_request(const nlohmann::json& json_body) {
    if (!json_body.contains("flightNumber") ||
        json_body["flightNumber"].is_null() ||
        json_body["flightNumber"].get<std::string>().empty()) {

        nlohmann::json error_response = {
            {"message", "Ошибка валидации данных"},
            {"errors", {
                {
                    {"field", "flightNumber"},
                    {"description", "Номер рейса обязателен для заполнения"}
                }
            }}
        };

        crow::response res(400, error_response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }

    if (!json_body.contains("price") ||
        !json_body["price"].is_number() ||
        json_body["price"].get<int>() <= 0) {

        nlohmann::json error_response = {
            {"message", "Ошибка валидации данных"},
            {"errors", {
                {
                    {"field", "price"},
                    {"description", "Стоимость должна быть положительным числом"}
                }
            }}
        };

        crow::response res(400, error_response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }

    if (!json_body.contains("paidFromBalance") ||
        !json_body["paidFromBalance"].is_boolean()) {

        nlohmann::json error_response = {
            {"message", "Ошибка валидации данных"},
            {"errors", {
                {
                    {"field", "paidFromBalance"},
                    {"description", "Поле paidFromBalance обязательно и должно быть булевым"}
                }
            }}
        };

        crow::response res(400, error_response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }

    return crow::response(200);
}

// Асинхронный HTTP клиент
pplx::task<web::json::value> GatewayController::call_service_async(
    const std::string& url,
    const web::http::method& method,
    const web::json::value& body,
    const std::map<std::string, std::string>& headers) {

    try {
        http_client client(to_string_t(url), client_config);
        http_request request(method);

        for (const auto& header : headers) {
            request.headers().add(to_string_t(header.first), to_string_t(header.second));
        }

        if (!body.is_null() && method != methods::GET && method != methods::DEL) {
            request.set_body(body);
        }

        return client.request(request).then([](http_response response) {
            if (response.status_code() >= 400) {
                throw std::runtime_error("HTTP error: " + std::to_string(response.status_code()));
            }

            if (response.status_code() == 204) {
                return pplx::task_from_result(web::json::value());
            }

            return response.extract_json();
            });

    }
    catch (const std::exception& e) {
        std::cerr << "Service call failed: " << e.what() << std::endl;
        throw;
    }
}

// Асинхронный клиент с аутентификацией
pplx::task<web::json::value> GatewayController::call_service_with_auth_async(
    const std::string& url,
    const web::http::method& method,
    const std::string& username,
    const web::json::value& body) {

    std::map<std::string, std::string> headers = {
        {"X-User-Name", username}
    };

    return call_service_async(url, method, body, headers);
}

// Синхронные обертки
web::json::value GatewayController::call_service_sync(
    const std::string& url,
    const web::http::method& method,
    const web::json::value& body,
    const std::map<std::string, std::string>& headers) {

    try {
        auto task = call_service_async(url, method, body, headers);
        return task.get();
    }
    catch (const std::exception& e) {
        throw;
    }
}

web::json::value GatewayController::call_service_with_auth_sync(
    const std::string& url,
    const web::http::method& method,
    const std::string& username,
    const web::json::value& body) {

    try {
        auto task = call_service_with_auth_async(url, method, username, body);
        return task.get();
    }
    catch (const std::exception& e) {
        throw;
    }
}

// GET /api/v1/flights
crow::response GatewayController::get_flights(const crow::request& req) {
    try {
        std::string page = req.url_params.get("page") ? std::string(req.url_params.get("page")) : "1";
        std::string size = req.url_params.get("size") ? std::string(req.url_params.get("size")) : "10";

        std::stringstream flight_url;
        flight_url << flight_service_url << "/api/v1/flights?page=" << page << "&size=" << size;

        auto flight_response = call_service_sync(flight_url.str());

        std::string response_str = to_utf8string(flight_response.serialize());

        crow::response res(200, response_str);
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in get_flights: " << e.what() << std::endl;
        return create_error_response(500, "Failed to get flights");
    }
}

// GET /api/v1/me - полная информация о пользователе
crow::response GatewayController::get_user_info(const crow::request& req) {
    try {
        std::string username = req.get_header_value("X-User-Name");

        if (username.empty()) {
            return create_error_response(400, "X-User-Name header is required");
        }

        nlohmann::json response;

        // 1. Получаем билеты пользователя
        try {
            std::string tickets_url = ticket_service_url + "/api/v1/tickets";
            auto tickets_response = call_service_with_auth_sync(tickets_url, methods::GET, username);

            nlohmann::json tickets_array;
            if (!tickets_response.is_null()) {
                std::string tickets_str = to_utf8string(tickets_response.serialize());
                tickets_array = nlohmann::json::parse(tickets_str);

                nlohmann::json full_tickets = nlohmann::json::array();

                for (auto& ticket : tickets_array) {
                    std::string flight_number = ticket["flightNumber"];
                    std::string ticket_uid = ticket["ticketUid"];

                    nlohmann::json flight_info;
                    try {
                        std::string flight_url = flight_service_url + "/api/v1/flights/" + flight_number;
                        auto flight_response = call_service_sync(flight_url, methods::GET);

                        std::string flight_str = to_utf8string(flight_response.serialize());
                        flight_info = nlohmann::json::parse(flight_str);
                    }
                    catch (...) {
                        flight_info = nlohmann::json::object();
                    }

                    nlohmann::json full_ticket;
                    full_ticket["ticketUid"] = ticket_uid;
                    full_ticket["flightNumber"] = flight_number;
                    full_ticket["price"] = ticket["price"];
                    full_ticket["status"] = ticket["status"];

                    if (flight_info.contains("fromAirport")) {
                        full_ticket["fromAirport"] = flight_info["fromAirport"];
                    }
                    else {
                        full_ticket["fromAirport"] = "";
                    }

                    if (flight_info.contains("toAirport")) {
                        full_ticket["toAirport"] = flight_info["toAirport"];
                    }
                    else {
                        full_ticket["toAirport"] = "";
                    }

                    if (flight_info.contains("date")) {
                        full_ticket["date"] = flight_info["date"];
                    }
                    else {
                        full_ticket["date"] = "";
                    }

                    full_tickets.push_back(full_ticket);
                }

                response["tickets"] = full_tickets;
            }
            else {
                response["tickets"] = nlohmann::json::array();
            }
        }
        catch (...) {
            response["tickets"] = nlohmann::json::array();
        }

        // 2. Получаем информацию о привилегиях
        try {
            std::string privilege_url = bonus_service_url + "/api/v1/privilege";
            auto privilege_response = call_service_with_auth_sync(privilege_url, methods::GET, username);

            int balance = get_json_int_field(privilege_response, "balance", 0);
            std::string status = get_json_string_field(privilege_response, "status", "BRONZE");

            response["privilege"] = {
                {"balance", balance},
                {"status", status}
            };
        }
        catch (...) {
            response["privilege"] = {
                {"balance", 0},
                {"status", "BRONZE"}
            };
        }

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in get_user_info: " << e.what() << std::endl;
        return create_error_response(500, "Failed to get user info");
    }
}

// GET /api/v1/tickets - все билеты пользователя
crow::response GatewayController::get_user_tickets(const crow::request& req) {
    try {
        std::string username = req.get_header_value("X-User-Name");

        if (username.empty()) {
            return create_error_response(400, "X-User-Name header is required");
        }

        std::string tickets_url = ticket_service_url + "/api/v1/tickets";
        auto tickets_response = call_service_with_auth_sync(tickets_url, methods::GET, username);

        std::string tickets_str = to_utf8string(tickets_response.serialize());
        nlohmann::json tickets_array = nlohmann::json::parse(tickets_str);

        nlohmann::json full_tickets = nlohmann::json::array();

        for (auto& ticket : tickets_array) {
            std::string flight_number = ticket["flightNumber"];
            std::string ticket_uid = ticket["ticketUid"];
			
            nlohmann::json flight_info;
            try {
                std::string flight_url = flight_service_url + "/api/v1/flights/" + flight_number;
                auto flight_response = call_service_sync(flight_url, methods::GET);

                std::string flight_str = to_utf8string(flight_response.serialize());
                flight_info = nlohmann::json::parse(flight_str);
            }
            catch (...) {
                flight_info = nlohmann::json::object();
            }

            nlohmann::json full_ticket;
            full_ticket["ticketUid"] = ticket_uid;
            full_ticket["flightNumber"] = flight_number;
            full_ticket["price"] = ticket["price"];
            full_ticket["status"] = ticket["status"];

            if (flight_info.contains("fromAirport")) {
                full_ticket["fromAirport"] = flight_info["fromAirport"];
            }
            else {
                full_ticket["fromAirport"] = "";
            }

            if (flight_info.contains("toAirport")) {
                full_ticket["toAirport"] = flight_info["toAirport"];
            }
            else {
                full_ticket["toAirport"] = "";
            }

            if (flight_info.contains("date")) {
                full_ticket["date"] = flight_info["date"];
            }
            else {
                full_ticket["date"] = "";
            }

            full_tickets.push_back(full_ticket);
        }

        crow::response res(200, full_tickets.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in get_user_tickets: " << e.what() << std::endl;
        return create_error_response(500, "Failed to get user tickets");
    }
}

// GET /api/v1/tickets/{ticketUid}
crow::response GatewayController::get_ticket_by_uid(const crow::request& req, const std::string& ticket_uid) {
    try {
        std::string username = req.get_header_value("X-User-Name");

        if (username.empty()) {
            return create_error_response(400, "X-User-Name header is required");
        }

        if (ticket_uid.empty()) {
            return create_error_response(400, "Ticket UID is required");
        }

        // 1. Получаем информацию о билете из Ticket Service
        std::string ticket_url = ticket_service_url + "/api/v1/tickets/" + ticket_uid;
        auto ticket_response = call_service_with_auth_sync(ticket_url, methods::GET, username);

        if (ticket_response.is_null()) {
            return create_error_response(404, "Ticket not found");
        }

        // 2. Извлекаем данные о билете
        std::string ticket_str = to_utf8string(ticket_response.serialize());
        nlohmann::json ticket_json = nlohmann::json::parse(ticket_str);

        std::string flight_number = ticket_json["flightNumber"];
        int price = ticket_json["price"];
        std::string status = ticket_json["status"];

        // 3. Получаем информацию о рейсе из Flight Service
        nlohmann::json flight_info;
        try {
            std::string flight_url = flight_service_url + "/api/v1/flights/" + flight_number;
            auto flight_response = call_service_sync(flight_url, methods::GET);

            std::string flight_str = to_utf8string(flight_response.serialize());
            flight_info = nlohmann::json::parse(flight_str);
        }
        catch (...) {
            // Если не удалось получить информацию о рейсе, используем пустые значения
            flight_info = nlohmann::json::object();
        }

        // 4. Формируем полный ответ
        nlohmann::json final_response;
        final_response["ticketUid"] = ticket_uid;
        final_response["flightNumber"] = flight_number;
        final_response["price"] = price;
        final_response["status"] = status;

        if (flight_info.contains("fromAirport")) {
            final_response["fromAirport"] = flight_info["fromAirport"];
        }
        else {
            final_response["fromAirport"] = "";
        }

        if (flight_info.contains("toAirport")) {
            final_response["toAirport"] = flight_info["toAirport"];
        }
        else {
            final_response["toAirport"] = "";
        }

        if (flight_info.contains("date")) {
            final_response["date"] = flight_info["date"];
        }
        else {
            final_response["date"] = "";
        }

        crow::response res(200, final_response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in get_ticket_by_uid: " << e.what() << std::endl;
        return create_error_response(404, "Ticket not found");
    }
}

// GET /api/v1/privilege
crow::response GatewayController::get_privilege_info(const crow::request& req) {
    try {
        std::string username = req.get_header_value("X-User-Name");

        if (username.empty()) {
            return create_error_response(400, "X-User-Name header is required");
        }

        std::string privilege_url = bonus_service_url + "/api/v1/privilege";
        auto privilege_response = call_service_with_auth_sync(privilege_url, methods::GET, username);

        if (privilege_response.is_null()) {
            return create_error_response(404, "Privilege not found");
        }

        std::string response_str = to_utf8string(privilege_response.serialize());
        nlohmann::json privilege_json = nlohmann::json::parse(response_str);

        nlohmann::json final_response;

        if (privilege_json.contains("balance")) {
            final_response["balance"] = privilege_json["balance"];
        }
        else {
            final_response["balance"] = 0;
        }

        if (privilege_json.contains("status")) {
            final_response["status"] = privilege_json["status"];
        }
        else {
            final_response["status"] = "BRONZE";
        }

        if (privilege_json.contains("history") && privilege_json["history"].is_array()) {
            final_response["history"] = privilege_json["history"];
        }
        else {
            final_response["history"] = nlohmann::json::array();
        }

        crow::response res(200, final_response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in get_privilege_info: " << e.what() << std::endl;
        return create_error_response(500, "Failed to get privilege info");
    }
}

// POST /api/v1/tickets - покупка билета
crow::response GatewayController::purchase_ticket(const crow::request& req) {
    try {
        std::string username = req.get_header_value("X-User-Name");

        if (username.empty()) {
            return create_error_response(400, "X-User-Name header is required");
        }

        auto json_body = nlohmann::json::parse(req.body);

        auto validation_response = validate_purchase_request(json_body);
        if (validation_response.code != 200) {
            return validation_response;
        }

        std::string flight_number = json_body["flightNumber"];
        int price = json_body["price"];
        bool paid_from_balance = json_body["paidFromBalance"];

        // 1. Проверяем существование рейса
        std::string flight_check_url = flight_service_url + "/api/v1/flights/" + flight_number;
        try {
            auto flight_response = call_service_sync(flight_check_url, methods::GET);
        }
        catch (const std::exception& e) {
            return create_error_response(400, "Flight not found");
        }

        // 2. Получаем информацию о текущем балансе привилегий
        std::string privilege_url = bonus_service_url + "/api/v1/privilege";
        int current_balance = 0;

        try {
            auto current_privilege = call_service_with_auth_sync(privilege_url, methods::GET, username);
            current_balance = get_json_int_field(current_privilege, "balance", 0);
        }
        catch (...) {
            current_balance = 0;
        }

        // 3. Рассчитываем оплату
        int paid_by_bonuses = 0;
        int paid_by_money = price;
        int bonus_delta = 0;
        std::string bonus_operation_type = "FILL_IN_BALANCE";

        if (paid_from_balance && current_balance > 0) {
            paid_by_bonuses = std::min(price, current_balance);
            paid_by_money = price - paid_by_bonuses;
            bonus_delta = -paid_by_bonuses; // Списание бонусов
            bonus_operation_type = "DEBIT_THE_ACCOUNT";
        }
        else {
            bonus_delta = static_cast<int>(price * 0.1);
            bonus_operation_type = "FILL_IN_BALANCE";
        }

        // 4. Создаем билет в Ticket Service
        web::json::value ticket_request;
        ticket_request[to_string_t("flightNumber")] = web::json::value::string(to_string_t(flight_number));
        ticket_request[to_string_t("price")] = web::json::value::number(price);

        std::string create_ticket_url = ticket_service_url + "/api/v1/tickets";
        auto ticket_response = call_service_with_auth_sync(create_ticket_url, methods::POST, username, ticket_request);

        if (ticket_response.is_null()) {
            return create_error_response(500, "Failed to create ticket");
        }

        std::string ticket_uid = get_json_string_field(ticket_response, "ticketUid", "");
        if (ticket_uid.empty()) {
            return create_error_response(500, "Failed to get ticket UID");
        }

        // 5. Обновляем баланс привилегий (если нужно)
        if (bonus_delta != 0) {
            web::json::value bonus_update;
            bonus_update[to_string_t("username")] = web::json::value::string(to_string_t(username));
            bonus_update[to_string_t("ticketUid")] = web::json::value::string(to_string_t(ticket_uid));
            bonus_update[to_string_t("balanceDiff")] = web::json::value::number(bonus_delta);

            if (bonus_delta > 0) {
                bonus_update[to_string_t("operationType")] = web::json::value::string(to_string_t("FILL_IN_BALANCE"));
            }
            else {
                bonus_update[to_string_t("operationType")] = web::json::value::string(to_string_t("DEBIT_THE_ACCOUNT"));
            }

            std::string bonus_update_url = bonus_service_url + "/api/v1/privilege/update";
            try {
                auto update_response = call_service_sync(bonus_update_url, methods::POST, bonus_update);
                std::cout << "Bonus update response: " << to_utf8string(update_response.serialize()) << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to update bonus balance for ticket: " << ticket_uid << ", error: " << e.what() << std::endl;
            }
        }

        // 6. Получаем обновленную информацию о привилегиях
        web::json::value updated_privilege;
        try {
            updated_privilege = call_service_with_auth_sync(privilege_url, methods::GET, username);
        }
        catch (...) {
            updated_privilege = web::json::value::object();
            updated_privilege[to_string_t("balance")] = web::json::value::number(current_balance + bonus_delta);
            updated_privilege[to_string_t("status")] = web::json::value::string(to_string_t("BRONZE"));
        }

        // 7. Получаем информацию о рейсе
        auto flight_info = call_service_sync(flight_check_url, methods::GET);

        // 8. Формируем финальный ответ
        nlohmann::json final_response;

        final_response["ticketUid"] = ticket_uid;
        final_response["flightNumber"] = flight_number;

        if (!flight_info.is_null()) {
            final_response["fromAirport"] = get_json_string_field(flight_info, "fromAirport", "");
            final_response["toAirport"] = get_json_string_field(flight_info, "toAirport", "");
            final_response["date"] = get_json_string_field(flight_info, "date", "");
        }

        final_response["price"] = price;
        final_response["paidByMoney"] = paid_by_money;
        final_response["paidByBonuses"] = paid_by_bonuses;
        final_response["status"] = "PAID";

        if (!updated_privilege.is_null()) {
            nlohmann::json privilege_json;
            privilege_json["balance"] = get_json_int_field(updated_privilege, "balance", 0);
            privilege_json["status"] = get_json_string_field(updated_privilege, "status", "BRONZE");

            final_response["privilege"] = privilege_json;
        }

        crow::response res(200, final_response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in purchase_ticket: " << e.what() << std::endl;
        return create_error_response(500, "Failed to purchase ticket");
    }
}

// DELETE /api/v1/tickets/{ticketUid} - возврат билета
crow::response GatewayController::refund_ticket(const crow::request& req, const std::string& ticket_uid) {
    try {
        std::string username = req.get_header_value("X-User-Name");

        if (username.empty()) {
            return create_error_response(400, "X-User-Name header is required");
        }

        if (ticket_uid.empty()) {
            return create_error_response(400, "Ticket UID is required");
        }

        // 1. Получаем информацию о билете
        std::string get_ticket_url = ticket_service_url + "/api/v1/tickets/" + ticket_uid;
        auto ticket_info = call_service_with_auth_sync(get_ticket_url, methods::GET, username);

        if (ticket_info.is_null()) {
            return create_error_response(404, "Ticket not found");
        }
		
        std::string status = get_json_string_field(ticket_info, "status", "");

        if (status == "CANCELED") {
            return create_error_response(400, "Ticket already canceled");
        }

        // 2. Получаем информацию о бонусной операции для этого билета
        std::string privilege_url = bonus_service_url + "/api/v1/privilege";
        auto privilege_info = call_service_with_auth_sync(privilege_url, methods::GET, username);

        bool bonus_operation_found = false;
        int bonus_diff_for_refund = 0;

        if (!privilege_info.is_null() && privilege_info.has_field(to_string_t("history"))) {
            auto history_value = get_json_array_as_value(privilege_info, "history");
            if (history_value.is_array()) {
                auto history_array = history_value.as_array();
                for (const auto& item : history_array) {
                    if (!item.is_null() && item.has_field(to_string_t("ticketUid"))) {
                        std::string history_ticket_uid = get_json_string_field(item, "ticketUid");
                        if (history_ticket_uid == ticket_uid) {
                            bonus_operation_found = true;

                            int original_diff = get_json_int_field(item, "balanceDiff", 0);
                            std::string operation_type = get_json_string_field(item, "operationType");

                            if (operation_type == "DEBIT_THE_ACCOUNT") {
                                bonus_diff_for_refund = -original_diff;
                            }
                            else if (operation_type == "FILL_IN_BALANCE") {
                                bonus_diff_for_refund = -original_diff;
                            }
                            break;
                        }
                    }
                }
            }
        }

        // 3. Обновляем баланс привилегий (если была операция)
        if (bonus_operation_found && bonus_diff_for_refund != 0) {
            web::json::value bonus_update;
            bonus_update[to_string_t("username")] = web::json::value::string(to_string_t(username));
            bonus_update[to_string_t("ticketUid")] = web::json::value::string(to_string_t(ticket_uid));
            bonus_update[to_string_t("balanceDiff")] = web::json::value::number(bonus_diff_for_refund);

            if (bonus_diff_for_refund > 0) {

                bonus_update[to_string_t("operationType")] = web::json::value::string(to_string_t("FILL_IN_BALANCE"));
            }
            else {

                bonus_update[to_string_t("operationType")] = web::json::value::string(to_string_t("DEBIT_THE_ACCOUNT"));
            }

            std::string bonus_update_url = bonus_service_url + "/api/v1/privilege/update";
            try {
                call_service_sync(bonus_update_url, methods::POST, bonus_update);
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to update bonus balance for refund: " << e.what() << std::endl;
            }
        }

        // 4. Помечаем билет как отмененный
        std::string cancel_ticket_url = ticket_service_url + "/api/v1/tickets/" + ticket_uid;
        call_service_with_auth_sync(cancel_ticket_url, methods::DEL, username);

        return crow::response(204);

    }
    catch (const std::exception& e) {
        std::cerr << "Error in refund_ticket: " << e.what() << std::endl;
        return create_error_response(500, "Failed to refund ticket");
    }
}

// Роутер
void GatewayController::router(crow::SimpleApp& app) {
    // Health check
    CROW_ROUTE(app, "/manage/health")
        .methods("GET"_method)
        ([this]() {
        return this->health_check();
            });

    // GET /api/v1/flights
    CROW_ROUTE(app, "/api/v1/flights")
        .methods("GET"_method)
        ([this](const crow::request& req) {
        return this->get_flights(req);
            });

    // GET /api/v1/me
    CROW_ROUTE(app, "/api/v1/me")
        .methods("GET"_method)
        ([this](const crow::request& req) {
        return this->get_user_info(req);
            });

    // GET /api/v1/tickets
    CROW_ROUTE(app, "/api/v1/tickets")
        .methods("GET"_method)
        ([this](const crow::request& req) {
        return this->get_user_tickets(req);
            });

    // POST /api/v1/tickets
    CROW_ROUTE(app, "/api/v1/tickets")
        .methods("POST"_method)
        ([this](const crow::request& req) {
        return this->purchase_ticket(req);
            });

    // GET /api/v1/tickets/{ticketUid}
    CROW_ROUTE(app, "/api/v1/tickets/<string>")
        .methods("GET"_method)
        ([this](const crow::request& req, const std::string& ticket_uid) {
        return this->get_ticket_by_uid(req, ticket_uid);
            });

    // DELETE /api/v1/tickets/{ticketUid}
    CROW_ROUTE(app, "/api/v1/tickets/<string>")
        .methods("DELETE"_method)
        ([this](const crow::request& req, const std::string& ticket_uid) {
        return this->refund_ticket(req, ticket_uid);
            });

    // GET /api/v1/privilege
    CROW_ROUTE(app, "/api/v1/privilege")
        .methods("GET"_method)
        ([this](const crow::request& req) {
        return this->get_privilege_info(req);
            });
}