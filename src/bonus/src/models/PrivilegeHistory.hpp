#pragma once
#include <string>
#include <nlohmann/json.hpp>

class PrivilegeHistory {
public:
    int id;
    int privilege_id;
    std::string ticket_uid;
    std::string datetime;
    int balance_diff;
    std::string operation_type;

    PrivilegeHistory() = default;

    nlohmann::json to_json() const {
        return {
            {"date", datetime},
            {"ticketUid", ticket_uid},
            {"balanceDiff", balance_diff},
            {"operationType", operation_type}
        };
    }
};