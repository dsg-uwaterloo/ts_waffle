//
// Created by Peter Pan on 2/13/2024.
//
// compilation command: g++ TS_value_master_test.cpp utils.cpp -lboost_serialization -o TS_value_master_test
#include "TS_value_master.h"
#include <cassert>
#include <iostream>
#include "TS_key_master.h"

int main() {
    // BinarySerializer Tests
    std::cout << "Testing BinarySerializer..." << std::endl;
    {
        std::vector<int> intData = {1, 2, 3, 4, 5};
        std::string serializedIntData = BinarySerializer::serialize(intData);
        auto deserializedIntData = BinarySerializer::deserialize<int>(serializedIntData);
        assert(intData == deserializedIntData);

        std::vector<float> floatData = {1.1f, 2.2f, 3.3f, 4.4f, 5.5f};
        std::string serializedFloatData = BinarySerializer::serialize(floatData);
        auto deserializedFloatData = BinarySerializer::deserialize<float>(serializedFloatData);
        assert(floatData == deserializedFloatData);

        std::cout << "BinarySerializer tests passed." << std::endl;
    }

    // TimeSeriesDataGenerator Tests
    std::cout << "Testing TimeSeriesDataGenerator..." << std::endl;
    {
        auto intData = TimeSeriesDataGenerator::generateData<int>(sizeof(int) * 5);
        assert(intData.size() == 5);

        auto floatData = TimeSeriesDataGenerator::generateData<float>(sizeof(float) * 5);
        assert(floatData.size() == 5);

        std::cout << "TimeSeriesDataGenerator tests passed." << std::endl;
    }

    // TimeSeriesDataMap Tests
    std::cout << "Testing TimeSeriesDataMap..." << std::endl;
    {
        //count time
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::string> keys = ItemIdGenerator::generate_item_ids(5000);
        int generation_interval = 1;
        size_t object_size = 64; // Assume size is enough for 5 element
        TimeSeriesDataMap tsDataMap(keys, generation_interval, object_size);
        for(int i = 0; i < 10000; i++) {
            std::tuple<std::string, long, std::string> result = tsDataMap.generate_TS_data();
            std::string key = std::get<0>(result);
            long timestamp = std::get<1>(result);
            std::string data = std::get<2>(result);

            assert(!key.empty() && !data.empty());
            std::cout << "Generated data for key: " << key << " at timestamp: " << timestamp << std::endl;
            assert(data.length()==object_size); //
        }
        auto end = std::chrono::high_resolution_clock::now();
        //print time duration
        std::cout << "TimeSeriesDataMap tests passed. Time duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }

    std::cout << "Testing DataPair Deserialization..." << std::endl;
    {
        auto data_pair = TimeSeriesDataMap::get_TS_datapair_from_file("../tracefiles/TS_data_binary.bin");
        assert(data_pair[0].size() == data_pair[1].size());
        std::cout << "Number of buckets: " << data_pair[0].size() << std::endl;
        for (int i = 0; i < data_pair[0].size(); i++){
            if (i % 9 == 0){
                std::string key = data_pair[0][i];
                std::cout << "Key: " << key << ", Values: ";
                std::string data_type = DataType::get_data_type(key);
                if (data_type == "int"){
                    auto deserializedData = BinarySerializer::deserialize<int>(data_pair[1][i]);
                    if (deserializedData.size() == 0) {
                        continue;
                    }
                    for (int j = 0; j < 10; j++){
                        std::cout << deserializedData[j] << " ";
                    }
                    std::cout << "..." << std::endl;
                }
                else if (data_type == "float"){
                    auto deserializedData = BinarySerializer::deserialize<float>(data_pair[1][i]);
                    if (deserializedData.size() == 0) {
                        continue;
                    }
                    for (int j = 0; j < 10; j++){
                        std::cout << deserializedData[j] << " ";
                    }
                    std::cout << "..." << std::endl;
                }
                else if (data_type == "bool"){
                    auto deserializedData = BinarySerializer::deserialize(data_pair[1][i]);
                    if (deserializedData.size() == 0) {
                        continue;
                    }
                    for (int j = 0; j < 10; j++){
                        std::cout << deserializedData[j] << " ";
                    }
                    std::cout << "..." << std::endl;
                }
            }
        }
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}
