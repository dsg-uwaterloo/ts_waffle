//
// Created by Peter Pan on 2/13/2024.
//

#ifndef WAFFLE_TS_VALUE_MASTER_H
#define WAFFLE_TS_VALUE_MASTER_H
#include <string>
#include <vector>
#include <cstring> // For memcpy
#include <type_traits> // For std::is_floating_point, etc.
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <chrono>
#include <iostream>
#include "utils.h"


class BinarySerializer {
public:
    // General template for arithmetic types excluding bool
    template<typename T>
    static typename std::enable_if<!std::is_same<T, bool>::value, std::string>::type serialize(const std::vector<T>& data) {
        std::string binaryData;
        binaryData.resize(data.size() * sizeof(T));
        std::memcpy(&binaryData[0], &data[0], binaryData.size());
        return binaryData;
    }

    // Specialization for bool
    static std::string serialize(const std::vector<bool>& data) {
        std::string binaryData;
        binaryData.reserve(data.size());
        for (bool val : data) {
            binaryData.push_back(val ? 1 : 0); // Convert bool to char
        }
        return binaryData;
    }

    // General template for arithmetic types excluding bool
    template<typename T>
    static typename std::enable_if<!std::is_same<T, bool>::value, std::vector<T>>::type deserialize(const std::string& binaryData) {
        std::vector<T> data(binaryData.size() / sizeof(T));
        std::memcpy(&data[0], &binaryData[0], binaryData.size());
        return data;
    }

    // Specialization for bool
    static std::vector<bool> deserialize(const std::string& binaryData) {
        std::vector<bool> data;
        data.reserve(binaryData.size());
        for (char c : binaryData) {
            data.push_back(c != 0); // Convert char to bool
        }
        return data;
    }
};

// Adjust TimeSeriesDataGenerator for bool specialization
class TimeSeriesDataGenerator {
public:
    template<typename T>
    static std::vector<T> generateData(size_t object_size) {
        static_assert(std::is_arithmetic<T>::value, "Supported types are integer, float, and Boolean");

        size_t num_elements = object_size / sizeof(T);
        if (object_size % sizeof(T) != 0) {
            throw std::invalid_argument("Object size does not align with data type size");
        }

        std::vector<T> data(num_elements);
        std::random_device rd;
        std::mt19937 gen(rd());

        if (std::is_floating_point<T>::value) {
            std::uniform_real_distribution<float> dist(-1000.0, 1000.0);
            for (size_t i = 0; i < num_elements; ++i) {
                data[i] = dist(gen);
            }
        } else if (std::is_integral<T>::value && !std::is_same<T, bool>::value) {
            std::uniform_int_distribution<int> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            for (size_t i = 0; i < num_elements; ++i) {
                data[i] = dist(gen);
            }
        } else if (std::is_same<T, bool>::value) {
            std::uniform_int_distribution<int> dist(0, 1);
            for (size_t i = 0; i < num_elements; ++i) {
                data[i] = dist(gen) != 0;
            }
        }

        return data;
    }
private:
    // Helper function template for filling data, specialized for bool
    template<typename T, typename Generator>
    static typename std::enable_if<!std::is_same<T, bool>::value, std::vector<T>>::type fillData(Generator& gen, std::vector<T>& data) {
        if (std::is_floating_point<T>::value) {
            std::uniform_real_distribution<T> dist(-1000.0, 1000.0);
            for (auto& elem : data) {
                elem = dist(gen);
            }
        } else if (std::is_integral<T>::value) {
            std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            for (auto& elem : data) {
                elem = dist(gen);
            }
        }
        return data;
    }

    // Specialization for bool
    template<typename T, typename Generator>
    static typename std::enable_if<std::is_same<T, bool>::value, std::vector<T>>::type fillData(Generator& gen, std::vector<T>& data) {
        std::uniform_int_distribution<int> dist(0, 1);
        for (auto& elem : data) {
            elem = dist(gen) != 0;
        }
        return data;
    }
};


class TimeSeriesDataMap {
public:
    int batch_size = 100;
    static std::string generateDataForKey(const std::string& key, int object_size){
        std::string dataType = DataType::get_data_type(key);
        std::string serializedData;

        if (dataType == "float") {
            auto data = TimeSeriesDataGenerator::generateData<float>(object_size);
            serializedData = BinarySerializer::serialize(data);
        } else if (dataType == "int") {
            auto data = TimeSeriesDataGenerator::generateData<int>(object_size);
            serializedData = BinarySerializer::serialize(data);
        } else if (dataType == "bool") {
            auto data = TimeSeriesDataGenerator::generateData<bool>(object_size); // Changed to bool for boolean type
            serializedData = BinarySerializer::serialize(data);
        } else {
            throw std::invalid_argument("Unsupported data type in key: " + dataType);
        }
        return serializedData;
    }


    TimeSeriesDataMap(const std::vector<std::string>& keys, int generation_interval, size_t object_size)
            : keys_(keys), generation_interval_(generation_interval), object_size_(object_size) {}

    TimeSeriesDataMap(const std::vector<std::string>& keys, int generation_interval, size_t object_size, int batch_size)
            : keys_(keys), generation_interval_(generation_interval), object_size_(object_size), batch_size(batch_size) {}

    std::pair<std::vector<std::string>, std::vector<std::string>> generate_batch_TS_data(int batch_size) {
        std::vector<std::string> keys;
        std::vector<std::string> data;
        for (int i = 0; i < batch_size; i++) {
            std::tuple<std::string, long, std::string> result = generate_TS_data();
            std::string key = std::get<0>(result);
            long timestamp = std::get<1>(result);
            std::string value = std::get<2>(result);
//            std::cout << "Generated data for key: " << key+"@"+std::to_string(timestamp) << " at timestamp: " << timestamp << std::endl;
            keys.push_back(key+"@"+std::to_string(timestamp));
            data.push_back(value);
        }
        return {keys, data};
    }
    std::pair<std::vector<std::string>, std::vector<std::string>> generate_batch_TS_data() {
        return generate_batch_TS_data(batch_size);
    }
    std::tuple<std::string, long, std::string> generate_TS_data() {
        while(true) {
            long now = UNIX_TIMESTAMP::current_time();
            for (auto const &last_generation_it: last_generation_time_) {
                long last_generation_time = last_generation_it.second;
                long duration_since_last_gen = now - last_generation_time;
                std::string key = last_generation_it.first;
                int size = DataType::get_data_type_size(key);
                int time_increase = object_size_ / size * generation_interval_;
                //print with probability 0.01
//                if (rand() % 100 == 0) {
//                    std::cout<<"key: "<<key<<std::endl;
//                    std::cout<<"last_generation_time: "<<last_generation_time<<std::endl;
//                    std::cout<<"duration_since_last_gen: "<<duration_since_last_gen<<std::endl;
//                    std::cout<<"time_increase: "<<time_increase<<std::endl;
//                }
                if (duration_since_last_gen >= time_increase) {
                    auto data = generateDataForKey(key);
                    last_generation_time_[key] = last_generation_time + time_increase; // Update last generation time
                    return {key, last_generation_time + generation_interval_, data};
                }
            }
            for (const auto &key: keys_) {
                auto last_generation_it = last_generation_time_.find(key);
                if (last_generation_it != last_generation_time_.end()) {
                    continue;
                } else {
                    std::string data = generateDataForKey(key);
                    last_generation_time_[key] = now; // Update last generation time
                    return {key, now, data};
                }
            }
            std::cout<<"No data needs to be generated at this time."<<std::endl;
        }
    }
private:

    std::string generateDataForKey(const std::string& key) const {
        return generateDataForKey(key, object_size_);
    }
    std::vector<std::string> keys_;
    double generation_interval_;
    size_t object_size_;
    std::unordered_map<std::string, long> last_generation_time_; // Use string for UNIX timestamp

};



#endif //WAFFLE_TS_VALUE_MASTER_H
