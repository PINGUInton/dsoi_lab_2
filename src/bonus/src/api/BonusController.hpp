#pragma once
#include <crow.h>
#include <nlohmann/json.hpp>
#include "../database/BonusRepository.hpp"

class BonusController {
private:
    BonusRepository& bonus_repository;

public:
    explicit BonusController(BonusRepository& repo) : bonus_repository(repo) {}

    void router(crow::SimpleApp& app);

private:
    crow::response health_check();

    crow::response get_privilege_info(const crow::request& req);

    crow::response update_privilege_balance(const crow::request& req,
        const std::string& username,
        const std::string& ticket_uid,
        int balance_diff,
        const std::string& operation_type);

    crow::response create_error_response(int status_code, const std::string& message);
};