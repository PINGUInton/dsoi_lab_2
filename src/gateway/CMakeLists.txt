cmake_minimum_required(VERSION 3.15)
project(GatewayService VERSION 1.0.0 LANGUAGES CXX)

# Conan 2.0 требует этот файл вместо conanbuildinfo.cmake
include(${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)

# Находим пакет через CMakeDeps (сгенерированный Conan)
find_package(asio REQUIRED)
find_package(libpqxx REQUIRED)
find_package(Crow REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cpprestsdk REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(gateway_service ${SOURCES})

# Линкуем с библиотеками
target_link_libraries(gateway_service asio::asio libpqxx::pqxx Crow::Crow nlohmann_json::nlohmann_json cpprestsdk::cpprestsdk)

target_include_directories(gateway_service PRIVATE src)
