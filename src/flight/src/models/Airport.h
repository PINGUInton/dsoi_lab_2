#pragma once


#include <string>
#include <nlohmann/json.hpp>

class Airport {
private:
    int id;
    std::string name;
    std::string city;
    std::string country;

public:
    Airport() = default;

    Airport(int id,
        const std::string& name,
        const std::string& city,
        const std::string& country)
        : id(id)
        , name(name)
        , city(city)
        , country(country)
    {}
    
    std::string get_full_name() const {
        return city + " " + name;
    }
    
    nlohmann::json to_json() const {
        return {
            {"id", id},
            {"name", name},
            {"city", city},
            {"country", country}
        };
    }

    // Из базы
    static Airport from_json(const nlohmann::json& j) {
        Airport airport;
        airport.id = j["id"];
        airport.name = j["name"];
        airport.city = j["city"];
        airport.country = j["country"];
        return airport;
    }
};