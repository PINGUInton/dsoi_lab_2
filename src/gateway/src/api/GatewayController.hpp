#pragma once
#include <crow.h>
#include <nlohmann/json.hpp>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <string>
#include <map>
#include <vector>

class GatewayController {
private:
    std::string flight_service_url;
    std::string ticket_service_url;
    std::string bonus_service_url;

    web::http::client::http_client_config client_config;

public:
    GatewayController(const std::string& flight_url,
        const std::string& ticket_url,
        const std::string& bonus_url)
        : flight_service_url(flight_url)
        , ticket_service_url(ticket_url)
        , bonus_service_url(bonus_url) {

        client_config.set_timeout(std::chrono::seconds(10));
    }

    void router(crow::SimpleApp& app);

private:
    // Health check
    crow::response health_check();

    // API endpoints
    crow::response get_flights(const crow::request& req);
    crow::response get_user_info(const crow::request& req);
    crow::response get_user_tickets(const crow::request& req);
    crow::response get_ticket_by_uid(const crow::request& req, const std::string& ticket_uid);
    crow::response get_privilege_info(const crow::request& req);
    crow::response purchase_ticket(const crow::request& req);
    crow::response refund_ticket(const crow::request& req, const std::string& ticket_uid);

    crow::response create_error_response(int status_code, const std::string& message);
    crow::response validate_purchase_request(const nlohmann::json& json_body);

    // HTTP клиенты
    pplx::task<web::json::value> call_service_async(
        const std::string& url,
        const web::http::method& method,
        const web::json::value& body = web::json::value(),
        const std::map<std::string, std::string>& headers = {});

    pplx::task<web::json::value> call_service_with_auth_async(
        const std::string& url,
        const web::http::method& method,
        const std::string& username,
        const web::json::value& body = web::json::value());

    // Синхронные обертки
    web::json::value call_service_sync(
        const std::string& url,
        const web::http::method& method = web::http::methods::GET,
        const web::json::value& body = web::json::value(),
        const std::map<std::string, std::string>& headers = {});

    web::json::value call_service_with_auth_sync(
        const std::string& url,
        const web::http::method& method,
        const std::string& username,
        const web::json::value& body = web::json::value());
};