#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "Airport.h"

class Flight {
private:
    int id_;
    std::string flight_number_;
    std::string datetime_;
    Airport from_airport_;
    Airport to_airport_;
    int price_;

public:
    Flight() = default;

    Flight(int id,
           const std::string& flight_number, 
           const std::string& datetime,
           const Airport& from_airport, const Airport& to_airport,
           int price)
        : id_(id)
        , flight_number_(flight_number)
        , datetime_(datetime)
        , from_airport_(from_airport)
        , to_airport_(to_airport)
        , price_(price)
    {}
    
    std::string get_datetime_string() const {
        return datetime_;
    }
    
    // Для ответа API
    nlohmann::json to_api_json() const {
        return {
            {"flightNumber", flight_number_},
            {"fromAirport", from_airport_.get_full_name()},
            {"toAirport", to_airport_.get_full_name()},
            {"date", get_datetime_string()},
            {"price", price_}
        };
    }
    
    nlohmann::json to_json() const {
        return {
            {"id", id_},
            {"flight_number", flight_number_},
            {"datetime", get_datetime_string()},
            {"from_airport", from_airport_.to_json()},
            {"to_airport", to_airport_.to_json()},
            {"price", price_}
        };
    }
    
    // Из базы
    static Flight from_json(const nlohmann::json& j) {
        Flight flight;
        flight.id_ = j["id"];
        flight.flight_number_ = j["flight_number"];
        flight.price_ = j["price"];
        return flight;
    }
};