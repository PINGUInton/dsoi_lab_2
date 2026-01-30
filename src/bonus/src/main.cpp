#include <iostream>
#include <string>
#include <crow.h>
#include "api/BonusController.hpp"
#include "database/BonusRepository.hpp"

int main() {
    crow::SimpleApp app;

    std::string db_connection_string = "postgresql://program:test@postgres:5432/privileges";
    std::cout << "Подключение Bonus Service к: " << db_connection_string << std::endl;

    try {
        BonusRepository bonus_repository(db_connection_string);

        if (!bonus_repository.is_connected()) {
            std::cerr << "Ошибка соединения с базой данных Bonus Service: " << std::endl;
            return 1;
        }

        std::cout << "Bonus Service: Repository initialized successfully" << std::endl;

        BonusController controller(bonus_repository);
        controller.router(app);

        int port = 8050;

        std::cout << "Запуск Bonus Service на порту: " << port << std::endl;

        app.port(port)
            .multithreaded()
            .run();

    }
    catch (const std::exception& e) {
        std::cerr << "Критическая ошибка сервиса Bonus Service: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
