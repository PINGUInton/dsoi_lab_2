#include "BonusRepository.hpp"
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

BonusRepository::BonusRepository(const std::string& connection_string) {
    try {
        connection = std::make_unique<pqxx::connection>(connection_string);
        std::cout << "Bonus Service: Connected to database successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Bonus Service: Database connection failed: " << e.what() << std::endl;
        throw;
    }

    try {
        pqxx::work txn(*connection);

        std::string sql = "";

        auto result = txn.exec("SELECT count(*) FROM information_schema.tables WHERE table_schema = 'public'");
        sql.clear();

        if (result[0][0].as<int>() == 0) {
            sql.clear();
            sql = "CREATE TABLE privilege\
                (\
                    id       SERIAL PRIMARY KEY,\
                    username VARCHAR(80) NOT NULL UNIQUE,\
                    status   VARCHAR(80) NOT NULL DEFAULT 'BRONZE'\
                    CHECK(status IN('BRONZE', 'SILVER', 'GOLD')),\
                    balance  INT\
                    );";
            result = txn.exec(sql);

            sql.clear();
            sql = "CREATE TABLE privilege_history\
                (\
                    id             SERIAL PRIMARY KEY,\
                    privilege_id   INT REFERENCES privilege(id),\
                    ticket_uid     uuid        NOT NULL,\
                    datetime       TIMESTAMP   NOT NULL,\
                    balance_diff   INT         NOT NULL,\
                    operation_type VARCHAR(20) NOT NULL\
                    CHECK(operation_type IN('FILL_IN_BALANCE', 'DEBIT_THE_ACCOUNT'))\
                    );";
            result = txn.exec(sql);
            
            sql.clear();
            sql = "INSERT INTO privilege (username, status, balance) VALUES ('Test Max', 'GOLD', 1500);";
            result = txn.exec(sql);

            sql.clear();
            sql = "INSERT INTO privilege_history (privilege_id, ticket_uid, datetime, balance_diff, operation_type)\
                VALUES ('1', '049161bb-badd-4fa8-9d90-87c9a82b0668', '2021-10-08T19:59:19Z', '1500', 'FILL_IN_BALANCE');";
            result = txn.exec(sql);
        }
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Create table database and dataset error: " << e.what() << std::endl;
    }

}

BonusRepository::~BonusRepository() {
    if (connection && connection->is_open()) {
        connection->close();
    }
}

bool BonusRepository::connect() {
    return connection && connection->is_open();
}

bool BonusRepository::is_connected() const {
    return connection && connection->is_open();
}

std::string BonusRepository::get_privilege_status(int balance) {
    if (balance >= 10000) return "GOLD";
    if (balance >= 5000) return "SILVER";
    return "BRONZE";
}

Privilege BonusRepository::create_privilege_from_row(const pqxx::row& row) {
    try {
        Privilege privilege;
        privilege.id = row["id"].as<int>();
        privilege.username = row["username"].as<std::string>();
        privilege.balance = row["balance"].as<int>();
        privilege.status = row["status"].as<std::string>();
        return privilege;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating privilege from row: " << e.what() << std::endl;
        throw;
    }
}

PrivilegeHistory BonusRepository::create_history_from_row(const pqxx::row& row) {
    try {
        PrivilegeHistory history;
        history.id = row["id"].as<int>();
        history.privilege_id = row["privilege_id"].as<int>();
        history.ticket_uid = row["ticket_uid"].as<std::string>();
        history.datetime = row["datetime"].as<std::string>();
        history.balance_diff = row["balance_diff"].as<int>();
        history.operation_type = row["operation_type"].as<std::string>();
        return history;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating history from row: " << e.what() << std::endl;
        throw;
    }
}

std::optional<Privilege> BonusRepository::get_privilege_by_username(const std::string& username) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }

        pqxx::work txn(*connection);

        std::string sql = R"(
            SELECT id, username, balance, status
            FROM privilege
            WHERE username = $1
        )";

        auto result = txn.exec(sql, pqxx::params{ username });
        txn.commit();

        if (result.empty()) {
            return std::nullopt;
        }

        return create_privilege_from_row(result[0]);

    }
    catch (const std::exception& e) {
        std::cerr << "Error getting privilege: " << e.what() << std::endl;
        throw;
    }
}

Privilege BonusRepository::create_privilege(const std::string& username, int initial_balance) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }

        std::string status = get_privilege_status(initial_balance);

        pqxx::work txn(*connection);

        std::string sql = R"(
            INSERT INTO privilege (username, balance, status)
            VALUES ($1, $2, $3)
            RETURNING id, username, balance, status
        )";

        auto result = txn.exec(sql,
            pqxx::params{ username, initial_balance, status }
        );

        if (result.empty()) {
            throw std::runtime_error("Failed to create privilege");
        }

        Privilege privilege = create_privilege_from_row(result[0]);
        txn.commit();

        return privilege;

    }
    catch (const std::exception& e) {
        std::cerr << "Error creating privilege: " << e.what() << std::endl;
        throw;
    }
}

bool BonusRepository::update_privilege_balance(const std::string& username, int balance_diff) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }

        pqxx::work txn(*connection);

        // Получаем текущий баланс
        std::string select_sql = "SELECT balance FROM privilege WHERE username = $1";
        auto select_result = txn.exec(select_sql, pqxx::params{ username });

        if (select_result.empty()) {
            return false;
        }

        int current_balance = select_result[0]["balance"].as<int>();
        int new_balance = current_balance + balance_diff;

        if (new_balance < 0) {
            return false;
        }

        std::string status = get_privilege_status(new_balance);

        std::string update_sql = R"(
            UPDATE privilege
            SET balance = $1, status = $2
            WHERE username = $3
        )";

        txn.exec(update_sql,
            pqxx::params{ new_balance, status, username }
        );

        txn.commit();
        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Error updating privilege balance: " << e.what() << std::endl;
        throw;
    }
}

std::vector<PrivilegeHistory> BonusRepository::get_privilege_history(int privilege_id) {
    std::vector<PrivilegeHistory> history;

    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }

        pqxx::work txn(*connection);

        std::string sql = R"(
            SELECT id, privilege_id, ticket_uid, datetime, balance_diff, operation_type
            FROM privilege_history
            WHERE privilege_id = $1
            ORDER BY datetime DESC
        )";

        auto result = txn.exec(sql, pqxx::params{ privilege_id });
        txn.commit();

        for (const auto& row : result) {
            history.push_back(create_history_from_row(row));
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Error getting privilege history: " << e.what() << std::endl;
        throw;
    }

    return history;
}

void BonusRepository::add_privilege_history(int privilege_id, const std::string& ticket_uid,
    int balance_diff, const std::string& operation_type) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }

        pqxx::work txn(*connection);

        std::cout << "DEBUG: Adding history record" << std::endl;

        std::string sql = R"(
            INSERT INTO privilege_history (privilege_id, ticket_uid, datetime, balance_diff, operation_type)
            VALUES ($1, $2, CURRENT_TIMESTAMP, $3, $4)
        )";

        txn.exec(sql,
            pqxx::params{ privilege_id, ticket_uid, balance_diff, operation_type }
        );

        txn.commit();
        std::cout << "DEBUG: History record added successfully" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error adding privilege history: " << e.what() << std::endl;
        throw;
    }
}

bool BonusRepository::update_privilege_balance(const std::string& username,
    const std::string& ticket_uid,
    int balance_diff,
    const std::string& operation_type) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }

        std::string valid_operation_type = operation_type;
        if (operation_type == "FILLED_BY_MONEY") {
            valid_operation_type = "FILL_IN_BALANCE";
        }

        auto privilege_opt = get_privilege_by_username(username);
        int privilege_id;

        if (!privilege_opt) {
            Privilege new_privilege = create_privilege(username, 0);
            privilege_id = new_privilege.id;
        }
        else {
            privilege_id = privilege_opt.value().id;
        }

        if (!update_privilege_balance(username, balance_diff)) {
            return false;
        }

        add_privilege_history(privilege_id, ticket_uid, balance_diff, operation_type);

        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in combined update: " << e.what() << std::endl;
        throw;
    }
}