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
#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>

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
        std::vector<T> data(binaryData.size() / sizeof(T) + 1);
        std::memcpy(&data[0], &binaryData[0], binaryData.size());
        std::vector<T> sliced_data(data.begin(), data.end() - 1);
        return sliced_data;
        
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
    int current_generating_item_index=0;
    int batch_size = 100;
    std::string generateDataForKey(const std::string& key){
        // return default_value;
        std::string serializedData;
        std::string dataType = DataType::get_data_type(key);

        if (dataType == "float") {
            auto data = TimeSeriesDataGenerator::generateData<float>(object_size_);
            serializedData = BinarySerializer::serialize(data);
        } else if (dataType == "int") {
            auto data = TimeSeriesDataGenerator::generateData<int>(object_size_);
            serializedData = BinarySerializer::serialize(data);
        } else if (dataType == "bool") {
            auto data = TimeSeriesDataGenerator::generateData<bool>(object_size_); // Changed to bool for boolean type
            serializedData = BinarySerializer::serialize(data);
        } else {
            throw std::invalid_argument("Unsupported data type in key: " + dataType);
        }
        return serializedData;
    }


    TimeSeriesDataMap(const std::vector<std::string>& keys, int generation_interval, size_t object_size)
            : keys_(keys), generation_interval_(generation_interval), object_size_(object_size) {}

    TimeSeriesDataMap(const std::vector<std::string>& keys, int generation_interval, size_t object_size, int batch_size)
            : keys_(keys), generation_interval_(generation_interval), object_size_(object_size), batch_size(batch_size) {
        std::string return_dummy(object_size,'a');
        default_value=return_dummy;
    }

    typedef std::vector<std::vector<std::string>> DataPair;

    // Serialization function
    template <class Archive>
    void serialize(Archive &ar, DataPair &data_pair, const unsigned int version)
    {
        ar & data_pair;
    }

    std::pair<std::string, std::string> generate_TS_tracefile_put_query(std::string key, int start_timestamp)
    {
        std::tuple<std::string, long, std::string> result = generate_TS_data();
        std::string value = generateDataForKey(key);
        return {key + "@" + std::to_string(start_timestamp), value};
    }

    std::pair<std::string, std::string> generate_TS_tracefile_get_query(std::string key, long latest_timestamp)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Exponential distribution with rate parameter (lambda)
        std::exponential_distribution<> exp_dist(5.0); // lambda = 5.0 for decay
        
        // Function to map a continuous exponential sample to an integer in the range [0, latest_timestamp]
        auto sample_exponential = [&](std::mt19937 &gen, long latest_timestamp) {
            double sample = exp_dist(gen);
            double scaled_sample = (1.0 - std::exp(-5.0 * sample)) * (latest_timestamp + 1);
            scaled_sample = 1700000000 + scaled_sample * (latest_timestamp - 1700000000) / (latest_timestamp + 1);
            return std::min(static_cast<long>(scaled_sample), latest_timestamp);
        };
        
        // Sample two integers
        long first_int = sample_exponential(gen, latest_timestamp);
        long second_int = sample_exponential(gen, latest_timestamp);
        
        // Ensure they are in ascending order
        if (first_int > second_int) {
            std::swap(first_int, second_int);
        }

        return {key + "@" + std::to_string(first_int) + "@" + std::to_string(second_int), ""};
    }

    void generate_TS_tracefile_binary(int row_count) {
        int batch_count = row_count / batch_size;
        std::cout << "batchsize" << batch_size << std::endl;
        std::vector<std::string> keys;
        std::vector<std::string> data;
        long latest_timestamp = 1700000000;
        int put_query_percentage = 50;
        for (int i = 0; i < row_count;) {
            int rand_num = std::rand() % 100;
            std::pair<std::string, std::string> result;
            if (rand_num < put_query_percentage || i == 0)
            {
                for (int k = 0; k < keys_.size() / batch_size; k++) {
                    // generate batch_size number of put queries
                    for (int j = 0; j < batch_size; j++) {
                        result = generate_TS_tracefile_put_query(keys_[k*batch_size + j], latest_timestamp);
                        keys.push_back(result.first);
                        data.push_back(result.second);
                        std::cout << "PUT " << result.first << std::endl;
                    }
                }
                // assume each data point spans one time unit, at most {object_size_} data points per bucket
                latest_timestamp += object_size_;
                i += keys_.size();
            }
            else
            {
                for (int k = 0; k < keys_.size() / batch_size; k++) {
                    for (int j = 0; j < batch_size; j++) {
                        // generate batch_size number of get queries
                        result = generate_TS_tracefile_get_query(keys_[k*batch_size+j], latest_timestamp-1);
                        keys.push_back(result.first);
                        data.push_back(result.second);
                        std::cout << "GET " << result.first << std::endl;
                    }
                }
                i += keys_.size();
            }
        }

        DataPair data_pair = {keys, data};
        // Serialize data to a file
        std::ofstream outFile("../tracefiles/TS_data_binary.bin", std::ios::binary);
        if (!outFile){
            std::cerr << "Failed to open file for writing." << std::endl;
            exit(1);
        }
        boost::archive::binary_oarchive oa(outFile);
        oa << data_pair;
        outFile.close();
        std::cout << "Data serialized using Boost Serialization." << std::endl;
    }

    static std::vector<std::vector<std::string>> get_TS_datapair_from_file(std::string filepath) {
        // Deserialize data from a file
        std::ifstream inFile(filepath, std::ios::binary);
        if (!inFile){
            std::cerr << "Failed to open file for reading." << std::endl;
            exit(1);
        }
        DataPair data_pair;
        boost::archive::binary_iarchive ia(inFile);
        ia >> data_pair;
        inFile.close();
        std::cout << "Data deserialized using Boost Serialization." << std::endl;
        return data_pair;
    }

    std::pair<std::vector<std::string>, std::vector<std::string>> generate_batch_TS_data(int batch_size)
    {
        std::vector<std::string> keys;
        std::vector<std::string> data;
        for (int i = 0; i < batch_size; i++) {
            std::tuple<std::string, long, std::string> result = generate_TS_data();
            std::string key = std::get<0>(result);
            long timestamp = std::get<1>(result);
            std::string value = std::get<2>(result);
            // std::cout << "Generated data for key: " << key+"@"+std::to_string(timestamp) << " at timestamp: " << timestamp << std::endl;
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
                int time_increase =1;// object_size_ / size * generation_interval_;
                if (duration_since_last_gen >= time_increase) {
                    auto data = generateDataForKey(key);
                    last_generation_time_[key] = last_generation_time + time_increase; // Update last generation time
                    // std::cout << "Generated data for key: " << key << std::endl;
                    return {key, last_generation_time + time_increase, data};
                }
            }
            if (last_generation_time_.size()==keys_.size()){
                continue;
            }
            if (rand()%30==0){
                std::cout<<"Important Info - Used Key Size: "<<last_generation_time_.size()<<std::endl;
            }
            for (const auto &key: keys_) {
                auto last_generation_it = last_generation_time_.find(key);

                if (last_generation_it != last_generation_time_.end()) {
                    continue;
                } else {
                    //print info
                    std::string data = generateDataForKey(key);
                    last_generation_time_[key] = now; // Update last generation time
//                    std::cout<<"New Key: "<<key<<" at time "<<now<<std::endl;

                    return {key, now, data};
                }
            }
//            std::cout<<"No data needs to be generated at this time."<<std::endl;
        }
    }
    std::tuple<std::string, long, std::string> generate_TS_data_fast() {
        if(++current_generating_item_index>=keys_.size()){
            current_time_stamp++;
            std::cout<<"Current Time Stamp: "<<current_time_stamp<<std::endl;
            current_generating_item_index %= keys_.size();

        }
        std::string key=keys_[current_generating_item_index];

        auto data = generateDataForKey(key);
        return {key, current_time_stamp, data};
    }

private:
    std::string default_value;
    long current_time_stamp=UNIX_TIMESTAMP::current_time();
    std::string generateDataForKey(const std::string& key) const {
        return generateDataForKey(key);
    }
    std::vector<std::string> keys_;
    double generation_interval_;
    size_t object_size_;
    std::unordered_map<std::string, long> last_generation_time_; // Use string for UNIX timestamp

};



#endif //WAFFLE_TS_VALUE_MASTER_H
