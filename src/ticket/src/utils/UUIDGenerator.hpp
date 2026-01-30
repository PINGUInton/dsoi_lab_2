#pragma once
#include <string>
#include <random>
#include <sstream>
#include <iomanip>

class UUIDGenerator {
private:
    static std::random_device rd;
    static std::mt19937 gen;
    static std::uniform_int_distribution<> dis;
    static std::uniform_int_distribution<> dis2;
    
public:
    static std::string generate_uuid_v4() {
        std::stringstream ss;
        
        // Генерация UUID 
        // Первые 8 символов
        for (int i = 0; i < 8; i++) {
            ss << std::hex << std::setw(1) << (dis(gen) & 0xF);
        }
        ss << "-";
        
        // Следующие 4 символа
        for (int i = 0; i < 4; i++) {
            ss << std::hex << std::setw(1) << (dis(gen) & 0xF);
        }
        ss << "-4"; // Версия 4
        
        // Еще 3 символа
        for (int i = 0; i < 3; i++) {
            ss << std::hex << std::setw(1) << (dis(gen) & 0xF);
        }
        ss << "-";
        
        // Вариант (8, 9, A, B)
        ss << std::hex << std::setw(1) << (8 + (dis2(gen) % 4));
        
        // Последние 3 символа
        for (int i = 0; i < 3; i++) {
            ss << std::hex << std::setw(1) << (dis(gen) & 0xF);
        }
        ss << "-";
        
        // Последние 12 символов
        for (int i = 0; i < 12; i++) {
            ss << std::hex << std::setw(1) << (dis(gen) & 0xF);
        }
        
        return ss.str();
    }
    
    static bool is_valid_uuid(const std::string& uuid) {
        // Простая проверка формата UUID
        if (uuid.length() != 36) return false;
        
        // Проверка позиций дефисов
        if (uuid[8] != '-' || uuid[13] != '-' || 
            uuid[18] != '-' || uuid[23] != '-') {
            return false;
        }
        
        // Проверка версии (4)
        if (uuid[14] != '4') return false;
        
        // Проверка варианта (8,9,A,B)
        char variant = uuid[19];
        if (!(variant == '8' || variant == '9' || 
              variant == 'a' || variant == 'b' ||
              variant == 'A' || variant == 'B')) {
            return false;
        }
        
        return true;
    }
};