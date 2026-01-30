#include "FlightRepository.h"
#include <stdexcept>
#include <iostream>

FlightRepository::FlightRepository(const std::string& connection_string) {
    try {
        connection = std::make_unique<pqxx::connection>(connection_string);
        std::cout << "Connected to database successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        throw;
    }


    try {
        pqxx::work txn(*connection);

        std::string sql = "";

        auto result = txn.exec("SELECT count(*) FROM information_schema.tables WHERE table_schema = 'public'");
        sql.clear();

        if (result[0][0].as<int>() == 0) {
            sql.clear();
            sql = "CREATE TABLE airport\
                (\
                    id      SERIAL PRIMARY KEY,\
                    name    VARCHAR(255),\
                    city    VARCHAR(255),\
                    country VARCHAR(255)\
                    );";
            result = txn.exec(sql);
            
            sql.clear();
            sql = "CREATE TABLE flight\
                (\
                    id              SERIAL PRIMARY KEY,\
                    flight_number   VARCHAR(20)              NOT NULL,\
                    datetime        TIMESTAMP WITH TIME ZONE NOT NULL,\
                    from_airport_id INT REFERENCES airport(id),\
                    to_airport_id   INT REFERENCES airport(id),\
                    price           INT                      NOT NULL\
                    );";
            result = txn.exec(sql);


            sql = "INSERT INTO airport (id, name, city, country) VALUES\
            (1, 'Шереметьево', 'Москва', 'Россия'),\
            (2, 'Пулково', 'Санкт-Петербург', 'Россия');";
            result = txn.exec(sql);

            sql.clear();
            sql = "INSERT INTO flight (id, flight_number, datetime, from_airport_id, to_airport_id, price) VALUES\
            (1, 'AFL031', '2021-10-08 20:00:00', 2, 1, 1500); ";
            result = txn.exec(sql);

        }

        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Create table database and test data error: " << e.what() << std::endl;
    }
}

FlightRepository::~FlightRepository() {
}

bool FlightRepository::connect() {
    return connection && connection->is_open();
}

bool FlightRepository::is_connected() const {
    return connection && connection->is_open();
}

Airport FlightRepository::get_airport_by_id(int id) {
    try {
        pqxx::work txn(*connection);

        auto result = txn.exec(
            "SELECT id, name, city, country FROM airport WHERE id = $1",
            pqxx::params{ id }
        );
        
        if (result.empty()) {
            throw std::runtime_error("Airport not found with id: " + std::to_string(id));
        }
        
        const auto& row = result[0];
        return Airport(
            row["id"].as<int>(),
            row["name"].as<std::string>(),
            row["city"].as<std::string>(),
            row["country"].as<std::string>()
        );
    } catch (const std::exception& e) {
        std::cerr << "Error getting airport: " << e.what() << std::endl;
        throw;
    }
}

std::string FlightRepository::parse_timestamp(const std::string& timestamp_str) {
    std::string datetime = timestamp_str.substr(0, 16);
    return datetime;
}

Flight FlightRepository::create_flight_from_row(const pqxx::row& row) {
    try {
        int id = row["id"].as<int>();
        std::string flight_number = row["flight_number"].as<std::string>();
        std::string datetime_str = row["datetime"].as<std::string>();
        int from_airport_id = row["from_airport_id"].as<int>();
        int to_airport_id = row["to_airport_id"].as<int>();
        int price = row["price"].as<int>();
        

        Airport from_airport = get_airport_by_id(from_airport_id);
        Airport to_airport = get_airport_by_id(to_airport_id);
        
        std::string datetime = parse_timestamp(datetime_str);
        
        return Flight(id, flight_number, datetime, from_airport, to_airport, price);
    } catch (const std::exception& e) {
        std::cerr << "Error creating flight from row: " << e.what() << std::endl;
        throw;
    }
}

std::vector<Flight> FlightRepository::get_all_flights(int page, int page_size) {
    std::vector<Flight> flights;
    
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }
        
        pqxx::work txn(*connection);

        int offset = (page - 1) * page_size;
        
        std::string sql = "SELECT id, flight_number, datetime, from_airport_id, to_airport_id, price FROM flight ORDER BY datetime ASC LIMIT $1 OFFSET $2";
        
        auto result = txn.exec(sql,
            pqxx::params{ page_size, offset });
        txn.commit();
        
        for (const auto& row : result) {
            flights.push_back(create_flight_from_row(row));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting all flights: " << e.what() << std::endl;
        throw;
    }
    
    return flights;
}

std::optional<Flight> FlightRepository::get_flight_by_number(const std::string& flight_number) {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }
        
        pqxx::work txn(*connection);
        
        std::string sql = "SELECT id, flight_number, datetime, from_airport_id, to_airport_id, price FROM flight WHERE flight_number = $1";
        
        auto result = txn.exec(sql, 
            pqxx::params{ flight_number });
        txn.commit();
        
        if (result.empty()) {
            return std::nullopt;
        }

        Flight flight = create_flight_from_row(result[0]);
         
        return flight;
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting flight by number: " << e.what() << std::endl;
        throw;
    }
}

int FlightRepository::get_total_flights_count() {
    try {
        if (!is_connected()) {
            throw std::runtime_error("Database not connected");
        }
        
        pqxx::work txn(*connection);
        
        std::string sql = "SELECT COUNT(*) as count FROM flight";
        auto result = txn.exec(sql);
        
        if (result.empty()) {
            return 0;
        }
        
        int count = result[0]["count"].as<int>();
        txn.commit();
        
        return count;
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting total flights count: " << e.what() << std::endl;
        throw;
    }
}


FlightRepository::PaginationResult FlightRepository::get_flights_paginated(int page, int page_size) {
    PaginationResult result;
    
    try {
        if (page < 1) page = 1;
        if (page_size < 1) page_size = 10;
        if (page_size > 100) page_size = 100; 

        result.page = page;
        result.page_size = page_size;
        result.total_count = get_total_flights_count();
        result.flights = get_all_flights(page, page_size);
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting paginated flights: " << e.what() << std::endl;
        throw;
    }
    
    return result;
}