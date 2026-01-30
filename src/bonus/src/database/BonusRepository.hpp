#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <pqxx/pqxx>
#include "../models/Privilege.hpp"
#include "../models/PrivilegeHistory.hpp"

class BonusRepository {
private:
    std::unique_ptr<pqxx::connection> connection;

public:
    BonusRepository(const std::string& connection_string);
    ~BonusRepository();

    bool connect();
    bool is_connected() const;

    std::optional<Privilege> get_privilege_by_username(const std::string& username);
    Privilege create_privilege(const std::string& username, int initial_balance = 0);
    bool update_privilege_balance(const std::string& username, int balance_diff);

    std::vector<PrivilegeHistory> get_privilege_history(int privilege_id);
    void add_privilege_history(int privilege_id, const std::string& ticket_uid,
        int balance_diff, const std::string& operation_type);

    bool update_privilege_balance(const std::string& username,
        const std::string& ticket_uid,
        int balance_diff,
        const std::string& operation_type);

private:
    Privilege create_privilege_from_row(const pqxx::row& row);
    PrivilegeHistory create_history_from_row(const pqxx::row& row);

    std::string get_privilege_status(int balance);
};