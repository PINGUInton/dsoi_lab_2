#pragma once
#include <crow.h>
#include <nlohmann/json.hpp>
#include "../database/TicketRepository.hpp"

class TicketController {
private:
    TicketRepository& ticket_repository;
    
public:
    explicit TicketController(TicketRepository& repo) : ticket_repository(repo) {}
    
    // Роутер
    void router(crow::SimpleApp& app);
    
private:
    // Обработчики запросов
    crow::response health_check();
    
    // Получить все билеты пользователя
    crow::response get_user_tickets(const crow::request& req);
    
    // Получить конкретный билет
    crow::response get_ticket_by_uid(const crow::request& req, const std::string& ticket_uid);
    
    // Создать новый билет
    crow::response create_ticket(const crow::request& req);
    
    // Возврат билета
    crow::response canceled_ticket(const crow::request& req, const std::string& ticket_uid);



    crow::response validate_ticket_creation(const nlohmann::json& json_body);
    crow::response create_error_response(int status_code, const std::string& message);
    crow::response create_error_array_response(const std::string err_type);
};