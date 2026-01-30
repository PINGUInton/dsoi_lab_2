#ifdef _MSC_VER
#define _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
#endif

#include <iostream>
#include <string>
#include <crow.h>
#include "api/GatewayController.hpp"

int main() {
    crow::SimpleApp app;

    // URL сервисов
    std::string flight_service_url = "http://flight:8060";
    std::string ticket_service_url = "http://ticket:8070";
    std::string bonus_service_url = "http://bonus:8050";

    std::cout << "Запуск Gateway Service с конфигурацией: " << std::endl;
    std::cout << "  Flight Service: " << flight_service_url << std::endl;
    std::cout << "  Ticket Service: " << ticket_service_url << std::endl;
    std::cout << "  Bonus Service: " << bonus_service_url << std::endl;

    try {
        GatewayController controller(flight_service_url, ticket_service_url, bonus_service_url);
        controller.router(app);

        int port = 8080;

        std::cout << "Запуск Gateway Service на порте: " << port << std::endl;

        app.port(port)
            .multithreaded()
            .run();

    }
    catch (const std::exception& e) {
        std::cerr << "Критическая ошибка сервиса Gateway Service: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
