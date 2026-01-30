#include "FlightController.h"
#include <sstream>

// Health check
crow::response FlightController::health_check() {
    try {
        if (!flight_repository.is_connected()) {
            return crow::response(500, "Database not connected");
        }
        
        nlohmann::json response = {
            {"status", "OK"}
        };

        return crow::response(200, response.dump());
        
    } catch (const std::exception& e) {
        return crow::response(500);
    }
}

nlohmann::json FlightController::create_pagination_response(const FlightRepository::PaginationResult& result) {
    nlohmann::json response = {
        {"page", result.page},
        {"pageSize", result.page_size},
        {"totalElements", result.total_count}
    };
    
    nlohmann::json items = nlohmann::json::array();

    for (const auto& flight : result.flights) {
        items.push_back(flight.to_api_json());
    }
    
    response["items"] = items;
    
    return response;
}

// GET /api/v1/flights?page= &size= 
crow::response FlightController::get_flights(const crow::request& req) {
    try {
        int page = 1;
        int size = 10;
        
        if (req.url_params.get("page")) {
            std::string page_str = req.url_params.get("page");
            
            page = std::stoi(page_str);
            
            if (page < 1) page = 1;
        }
        
        if (req.url_params.get("size")) {
            std::string size_str = req.url_params.get("size");
            
            size = std::stoi(size_str);
            
            if (size < 1) size = 1;
            
            if (size > 100) size = 100; 
        }
        
        auto result = flight_repository.get_flights_paginated(page, size);
        
        auto response_json = create_pagination_response(result);
        
        crow::response res(200, response_json.dump());
        res.set_header("Content-Type", "application/json");
        
        return res;
        
    } catch (const std::invalid_argument& e) {
        nlohmann::json error = {
            {"message", "Invalid query parameters"},
            {"error", e.what()}
        };
        return crow::response(400, error.dump());
        
    } catch (const std::exception& e) {
        nlohmann::json error = {
            {"message", "Internal server error"},
            {"error", e.what()}
        };
        return crow::response(500, error.dump());
    }
}

// GET /api/v1/flights/{flightNumber}
crow::response FlightController::get_flight_by_number(const std::string& flight_number) {
    try {
        auto flight_opt = flight_repository.get_flight_by_number(flight_number);
        
        Flight flight = flight_opt.value();
        auto response_json = flight.to_api_json();
        
        crow::response res(200, response_json.dump());
        res.set_header("Content-Type", "application/json");
        
        return res;
        
    } catch (const std::exception& e) {
        return crow::response(500);
    }
}

void FlightController::router(crow::SimpleApp& app) {
    // health check
    CROW_ROUTE(app, "/manage/health")
        .methods("GET"_method)
        ([this]() {
        return this->health_check();
            });

    // Основной endpoint для получения рейсов
    CROW_ROUTE(app, "/api/v1/flights")
        .methods("GET"_method)
        ([this](const crow::request& req) {
        return this->get_flights(req);
            });

    // Endpoint для получения конкретного рейса
    CROW_ROUTE(app, "/api/v1/flights/<string>")
        .methods("GET"_method)
        ([this](const std::string& flight_number) {
        return this->get_flight_by_number(flight_number);
            });
}