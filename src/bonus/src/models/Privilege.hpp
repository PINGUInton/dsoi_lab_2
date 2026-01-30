#pragma once
#include <string>
#include <nlohmann/json.hpp>

class Privilege {
public:
    int id;
    std::string username;
    int balance;
    std::string status;

    Privilege() = default;

    Privilege(const std::string& username, int balance, const std::string& status)
        : username(username), balance(balance), status(status) {
    }

    nlohmann::json to_api_json() const {
        return {
            {"balance", balance},
            {"status", status}
        };
    }

    nlohmann::json to_full_json() const {
        return {
            {"balance", balance},
            {"status", status}
        };
    }
};