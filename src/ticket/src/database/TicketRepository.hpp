#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <pqxx/pqxx>
#include "../models/Ticket.hpp"

class TicketRepository {
private:
    std::unique_ptr<pqxx::connection> connection;
    
public:
    TicketRepository(const std::string& connection_string);
    ~TicketRepository();
    
    bool connect();
    bool is_connected() const;
    
    Ticket create_ticket(const std::string& username, 
                        const std::string& flight_number, 
                        int price,
                        const std::string status);
    
    std::optional<Ticket> get_ticket_by_uid(const std::string& ticket_uid);
    std::vector<Ticket> get_tickets_by_username(const std::string& username);
    
    bool update_ticket_status(const std::string& ticket_uid, const std::string& new_status);
    
    bool ticket_belongs_to_user(const std::string& ticket_uid, const std::string& username);
    
private:
    Ticket create_ticket_from_row(const pqxx::row& row);
};