#include <iostream>
#include <string>
#include <crow.h>
#include "api/FlightController.h"
#include "database/FlightRepository.h"

int main() {
    
    crow::SimpleApp app;
    
    std::string db_connection_string;
    db_connection_string = "postgresql://program:test@postgres:5432/flights";
    std::cout << db_connection_string << std::endl;
    
    try {
        FlightRepository flight_repository(db_connection_string);
        
        if (!flight_repository.connect()) {
            std::cerr << "Ошибка соединения с БД flights" << std::endl;
            return 1;
        }
        
        FlightController controller(flight_repository);
        
        controller.router(app);
        
        int port = 8060;
        
        std::cout << "Запуск сервиса Flight Service на порту " << port << std::endl;
       
        app.port(port)
           .multithreaded()  
           .run();
           
    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка сервиса Flight Service: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
