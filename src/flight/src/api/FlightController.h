#pragma once
#include <crow.h>
#include <nlohmann/json.hpp>
#include "../database/FlightRepository.h"

class FlightController {
private:
    FlightRepository& flight_repository;
    
public:
    explicit FlightController(FlightRepository& repo)
        : flight_repository(repo) 
    {}
    
    void router(crow::SimpleApp& app);
    
private:
    crow::response get_flights(const crow::request& req);
    crow::response get_flight_by_number(const std::string& flight_number);
    crow::response health_check();
    
    nlohmann::json create_pagination_response(const FlightRepository::PaginationResult& result);
};