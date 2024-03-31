#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>
#include <filesystem> // Include for filesystem operations
#include "TS_value_master.h"
#include "TS_key_master.h"

namespace fs = std::filesystem; // Abbreviation for std::filesystem

int main() {
    int num_keys = 1000;
    int num_items = 10;
    int num_per_item = num_keys / num_items;

    // Generate the relative file path
    std::string relative_tracefile_location = "tracefiles/TS_tracefile_" + std::to_string(num_keys) + ".txt";

    // Convert to absolute path
    fs::path absolute_tracefile_path = fs::absolute(relative_tracefile_location);

    // Ensure the directory exists
    fs::create_directories(absolute_tracefile_path.parent_path());

    std::vector<std::string> keys;
//    std::string value(1024, 'a');
    std::vector<std::string> values;
    // Assuming ItemIdGenerator::generate_item_ids() is implemented elsewhere
    std::vector<std::string> items = ItemIdGenerator::generate_item_ids(num_items);
    for (auto &item : items) {
        for (long i = 0; i < num_per_item; i++)
        {
            std::string key=item + "@" + std::to_string(1700000000 + i);
            std::string value (1024- key.length(), 'a');
            keys.push_back(key);
            values.push_back(key+value);
        }
    }
    assert(keys.size() == num_keys);

    // Write key and value to file in the format of "SET key value"
    std::ofstream output_file(absolute_tracefile_path, std::ios::out);
    if (!output_file.is_open()) {
        std::perror("Unable to open workload file");
    }
    for (int i = 0; i < num_keys; i++) {
        output_file << "SET " << keys[i] << " " << values[i] << std::endl;
    }
    output_file.close();

    // Print the absolute path of the generated file
    std::cout << "Generated tracefile at: " << absolute_tracefile_path << std::endl;

    std::string relative_tracefile_banchmark_location = "tracefiles/TS_tracefile_benchmark_" + std::to_string(num_keys) + ".txt";
    fs::path absolute_tracefile_benchmark_path = fs::absolute(relative_tracefile_banchmark_location);
    fs::create_directories(absolute_tracefile_benchmark_path.parent_path());
    //Write key to the file in the format of "GET key"
    std::ofstream output_file_benchmark(absolute_tracefile_benchmark_path, std::ios::out);
    if (!output_file_benchmark.is_open()) {
        std::perror("Unable to open workload file");
    }
    for (auto &key : keys) {
        output_file_benchmark << "GET " << key << std::endl;
    }
    output_file_benchmark.close();

    std::cout << "Generated benchmark tracefile at: " << absolute_tracefile_benchmark_path << std::endl;


    return 0;
}
