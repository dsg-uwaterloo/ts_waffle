// This file is used to generate time series data for testing.
// It generates buckets of time series data in the format of <key@timestamp, serialized_data_of_length_{object_size}>.
// run compilation command: g++ generate_TS_tracefile.cpp utils.cpp -lboost_serialization -o generate_TS_tracefile

#include <iostream>
#include <cassert>
#include <fstream>
#include <vector>
#include "TS_value_master.h"
#include "TS_key_master.h"

int main() {
    int no_items = 3;
    int generation_interval = 50;
    int object_size = 100;
    int client_batch_size = 2;
    int row_count = 500;  // generate {row_count} buckets of time series data

    std::vector<std::string> keys;
    ItemIdGenerator::read_item_ids(keys, "../tracefiles/TS_ItemID_10000.txt", no_items);
    auto timeSeriesDataMap = TimeSeriesDataMap(keys, generation_interval, object_size, client_batch_size);
    timeSeriesDataMap.generate_TS_tracefile_binary(row_count);

    return 0;
}
