#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <pqxx/pqxx>
#include "../models/Flight.h"

class FlightRepository {
private:
    std::unique_ptr<pqxx::connection> connection;
    
public:
    FlightRepository(const std::string& connection_string);
    ~FlightRepository();
    
    bool connect();
    bool is_connected() const;
    
    // /api/v1/flights
    std::vector<Flight> get_all_flights(int page = 1, int page_size = 10);
    
    // Проверка на существование рейса
    std::optional<Flight> get_flight_by_number(const std::string& flight_number);
    
    int get_total_flights_count();
    
    struct PaginationResult {
        std::vector<Flight> flights;
        int total_count;
        int page;
        int page_size;
    };
    
    PaginationResult get_flights_paginated(int page = 1, int page_size = 10);
    
private:
    Airport get_airport_by_id(int id);
    
    std::string parse_timestamp(const std::string& timestamp_str);
    
    Flight create_flight_from_row(const pqxx::row& row);
};