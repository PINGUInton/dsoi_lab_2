#include <iostream>
#include <string>
#include <crow.h>
#include "api/TicketController.hpp"
#include "database/TicketRepository.hpp"

int main() {
    
    crow::SimpleApp app;
    
    std::string db_connection_string;
    db_connection_string = "postgresql://program:test@postgres:5432/tickets";
    std::cout << db_connection_string << std::endl;
    
    try {
        TicketRepository ticket_repository(db_connection_string);
        
        if (!ticket_repository.connect()) {
            std::cerr << "Ошибка подключения Ticket Service к базе данных: " << std::endl;
            return 1;
        }
        
        std::cout << "Ticket Service: Repository initialized successfully" << std::endl;
        
        TicketController controller(ticket_repository);
        
        controller.router(app);
        
        int port = 8070;
    
        std::cout << "Запуск Ticket Serviceна порте: " << port << std::endl;
        
        app.port(port)
           .multithreaded()
           .run();
           
    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка Ticket Service: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
