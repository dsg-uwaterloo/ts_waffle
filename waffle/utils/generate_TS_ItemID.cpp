#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>
#include <filesystem> // Include for filesystem operations
#include "TS_value_master.h"
#include "TS_key_master.h"

namespace fs = std::filesystem; // Abbreviation for std::filesystem

int main() {

//    std::vector<std::string> items;
//    ItemIdGenerator::read_item_ids( items,"tracefiles/TS_ItemID_10000.txt",100);
//    for (auto &item : items) {
//        std::cout<<item<<std::endl;
//    }
//    std::cout<<items.size()<<std::endl;
//    return 0;

    int num_items = 1000000;

    // Generate the relative file path
    std::string relative_tracefile_location = "tracefiles/TS_ItemID_" + std::to_string(num_items) + ".txt";

    // Convert to absolute path
    fs::path absolute_tracefile_path = fs::absolute(relative_tracefile_location);

    // Assuming ItemIdGenerator::generate_item_ids() is implemented elsewhere
    std::vector<std::string> items = ItemIdGenerator::generate_item_ids(num_items);

    std::ofstream output_file(absolute_tracefile_path, std::ios::out);
    // Write key and value to file in the format of "SET key value"
    if (!output_file.is_open()) {
        std::perror("Unable to open workload file");
    }

    for (auto &item : items) {
        output_file << item << std::endl;
    }

    output_file.close();

    // Print the absolute path of the generated file
    std::cout << "Generated tracefile at: " << absolute_tracefile_path << std::endl;

    return 0;
}
