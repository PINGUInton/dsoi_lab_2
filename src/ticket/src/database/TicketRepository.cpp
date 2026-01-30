#include "TicketRepository.hpp"
#include "../utils/UUIDGenerator.hpp"
#include <stdexcept>
#include <iostream>

TicketRepository::TicketRepository(const std::string& connection_string) {
    try {
        connection = std::make_unique<pqxx::connection>(connection_string);
        std::cout << "TicketService: Connected to database successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "TicketService: Database connection failed: " << e.what() << std::endl;
        throw;
    }

    try {
        pqxx::work txn(*connection);

        std::string sql = "";

        auto result = txn.exec("SELECT count(*) FROM information_schema.tables WHERE table_schema = 'public'");
        sql.clear();

        if (result[0][0].as<int>() == 0) {
            sql.clear();
            sql = "CREATE TABLE ticket\
                (\
                    id            SERIAL PRIMARY KEY,\
                    ticket_uid    uuid UNIQUE NOT NULL,\
                    username      VARCHAR(80) NOT NULL,\
                    flight_number VARCHAR(20) NOT NULL,\
                    price         INT         NOT NULL,\
                    status        VARCHAR(20) NOT NULL\
                    CHECK(status IN('PAID', 'CANCELED'))\
                    );";
            result = txn.exec(sql);

            sql.clear();
            sql = "INSERT INTO ticket (ticket_uid, username, flight_number, price, status) VALUES ('049161bb-badd-4fa8-9d90-87c9a82b0668', 'Test Max', 'AFL031', '1500', 'PAID');";
            result = txn.exec(sql);
        }

        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Create table database error: " << e.what() << std::endl;
    }

}

TicketRepository::~TicketRepository() {}

bool TicketRepository::connect() {
    return connection && connection->is_open();
}

bool TicketRepository::is_connected() const {
    return connection && connection->is_open();
}

Ticket TicketRepository::create_ticket_from_row(const pqxx::row& row) {
    try {
        Ticket ticket;
        
        ticket.id = row["id"].as<int>();
        ticket.ticket_uid = row["ticket_uid"].as<std::string>();
        ticket.username = row["username"].as<std::string>();
        ticket.flight_number = row["flight_number"].as<std::string>();
        ticket.price = row["price"].as<int>();
        ticket.status = row["status"].as<std::string>();
        
        
        return ticket;
    } catch (const std::exception& e) {
        std::cerr << "Error creating ticket from row: " << e.what() << std::endl;
        throw;
    }
}

// Создание нового билета
Ticket TicketRepository::create_ticket(const std::string& username, 
                                      const std::string& flight_number, 
                                      int price, const std::string status) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }

        std::string ticket_uid = UUIDGenerator::generate_uuid_v4();
        
        pqxx::work txn(*connection);
        
        std::string sql = R"(
            INSERT INTO ticket (ticket_uid, username, flight_number, price, status)
            VALUES ($1, $2, $3, $4, $5)
            RETURNING id, ticket_uid, username, flight_number, price, status
        )";
        
        auto result = txn.exec(
            sql, 
            pqxx::params{ ticket_uid, username, flight_number, price, status}
        );
        
        if (result.empty()) {
            throw std::runtime_error("Failed to create ticket");
        }
        
        Ticket ticket = create_ticket_from_row(result[0]);
        txn.commit();
        
        return ticket;
        
    } catch (const std::exception& e) {
        throw;
    }
}

// Получить билет по UUID
std::optional<Ticket> TicketRepository::get_ticket_by_uid(const std::string& ticket_uid) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }
        
        if (!UUIDGenerator::is_valid_uuid(ticket_uid)) {
            std::cerr << "Invalid UUID format: " << ticket_uid << std::endl;
            return std::nullopt;
        }
        
        pqxx::work txn(*connection);
        
        std::string sql = R"(
            SELECT id, ticket_uid, username, flight_number, price, status
            FROM ticket
            WHERE ticket_uid = $1
        )";
        
        auto result = txn.exec(sql,
            pqxx::params{ ticket_uid }
        );

        txn.commit();

        if (result.empty()) {
            return std::nullopt;
        }
        
        Ticket ticket = create_ticket_from_row(result[0]);
        
        return ticket;
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting ticket by UID: " << e.what() << std::endl;
        throw;
    }
}

// Получить все билеты пользователя
std::vector<Ticket> TicketRepository::get_tickets_by_username(const std::string& username) {
    std::vector<Ticket> tickets;
    
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }
        
        pqxx::work txn(*connection);
        
        std::string sql = R"(
            SELECT id, ticket_uid, username, flight_number, price, status
            FROM ticket
            WHERE username = $1
        )";
        
        auto result = txn.exec(sql,
            pqxx::params{ username });
        txn.commit();

        for (const auto& row : result) {
            tickets.push_back(create_ticket_from_row(row));
        }
        
    } catch (const std::exception& e) {
        throw;
    }
    
    return tickets;
}

// Обновить статус билета
bool TicketRepository::update_ticket_status(const std::string& ticket_uid, 
                                          const std::string& new_status) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }
        
        pqxx::work txn(*connection);
        
        std::string sql = R"(
            UPDATE ticket
            SET status = $1
            WHERE ticket_uid = $2
            RETURNING id
        )";
        
        auto result = txn.exec(sql,
            pqxx::params{ new_status, ticket_uid }
        );
        txn.commit();
        
        return !result.empty(); 

    } catch (const std::exception& e) {
        std::cerr << "Error updating ticket status: " << e.what() << std::endl;
        throw;
    }
}

// Проверить, принадлежит ли билет пользователю
bool TicketRepository::ticket_belongs_to_user(const std::string& ticket_uid, 
                                            const std::string& username) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }
        
        auto ticket_opt = get_ticket_by_uid(ticket_uid);
        if (!ticket_opt) {
            return false;
        }
        
        return ticket_opt->username == username;
        
    } catch (const std::exception& e) {
        std::cerr << "Error checking ticket ownership: " << e.what() << std::endl;
        throw;
    }
}