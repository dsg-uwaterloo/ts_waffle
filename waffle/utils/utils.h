//
// Created by beich on 2/13/2024.
//

#ifndef WAFFLE_UTILS_H
#define WAFFLE_UTILS_H
#include <string>
#include <unordered_map>
#include <stdexcept>
class UNIX_TIMESTAMP {
public:
    static long int current_time() {
        time_t now = time(0);
        return now;
    };
};


class DataType{
    public:
    static std::unordered_map<std::string, std::size_t> type2size;

    static std::string get_data_type(std::string const& checking_key){
        size_t firstAmp = checking_key.find('&');
        if (firstAmp == std::string::npos) {
            return "string";
        }
        size_t secondAmp = checking_key.find('&', firstAmp + 1);
        if (secondAmp == std::string::npos) {
            return "string";
        }
        return checking_key.substr(firstAmp + 1, secondAmp - firstAmp - 1);
    }

    static size_t get_data_type_size(const std::string & key){
        std::string data_type = get_data_type(key);
        if (type2size.find(data_type) == type2size.end()) {
            throw std::invalid_argument("Unsupported data type in key: " + data_type);
        }
        return type2size[data_type];
    }
};


std::unordered_map<std::string, std::size_t> DataType::type2size = {
        {"int", sizeof(int)},
        {"float", sizeof(float)},
        {"bool", sizeof(bool)}
};

#endif //WAFFLE_UTILS_H
