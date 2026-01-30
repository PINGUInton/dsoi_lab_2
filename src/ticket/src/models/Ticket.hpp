#pragma once
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

class Ticket {
public:
    int id;
    std::string ticket_uid;
    std::string username;
    std::string flight_number;
    int price;
    std::string status;
    
    Ticket() = default;
    
    Ticket(const std::string& username, const std::string& flight_number, 
           int price, const std::string& status)
        : username(username)
        , flight_number(flight_number)
        , price(price)
        , status(std::string{"PAID"}) {
    }

    // Для API
    nlohmann::json to_api_json() const {
        return {
            {"ticketUid", ticket_uid},
            {"flightNumber", flight_number},
            {"price", price},
            {"status", status}
        };
    }
    
    static Ticket from_json(const nlohmann::json& j) {
        Ticket ticket;
        
        ticket.id = j["id"];
        ticket.flight_number = j["flightNumber"];
        ticket.price = j["price"];
        ticket.status = j["status"];
        
        return ticket;
    }
};