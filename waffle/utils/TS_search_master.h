//
// Created by Peter Pan on 2/15/2024.
//

#ifndef WAFFLE_TS_SEARCH_MASTER_H
#define WAFFLE_TS_SEARCH_MASTER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "TS_get_oldest_key.h"
class TS_search_master {
    static std::vector<std::string> search(const std::string &key, const std::unordered_map<std::string, int> &accessFreqs){
        //loop through accessFreqs and find the keys that have the same prefix as the key
        std::vector<std::string> keys;
        for (auto const& x : accessFreqs){
            if (x.first.find(key) == 0){
                keys.push_back(x.first);
            }
        }
        return keys;
    }
    static std::vector<std::string> filter_by_time_range(const std::vector<std::string> &keys, const long &start, const long &end){
        std::vector<std::string> returnKeys;
        for (auto const& key : keys){
            //get the timestamp from the key
            long timestamp = TS_get_oldest_key::get_timestamp(key);
            if (timestamp >= start && timestamp <= end){
                returnKeys.push_back(key);
            }
        }
    }
};
#endif //WAFFLE_TS_SEARCH_MASTER_H
