#include "UUIDGenerator.hpp"

std::random_device UUIDGenerator::rd;
std::mt19937 UUIDGenerator::gen(rd());
std::uniform_int_distribution<> UUIDGenerator::dis(0, 15);
std::uniform_int_distribution<> UUIDGenerator::dis2(0, 3);