#include "utils.h"

// Definition of `type2size`
std::unordered_map<std::string, std::size_t> DataType::type2size = {
        {"int", sizeof(int)},
        {"float", sizeof(float)},
        {"bool", sizeof(bool)}
};

//
// Created by beich on 3/21/2024.
//
