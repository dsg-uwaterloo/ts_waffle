#include "FrequencySmoother.hpp"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <algorithm>
bool freqCmp(std::pair<std::string, int> a, std::pair<std::string, int> b){ return (a.second == b.second ? a.first < b.first : a.second < b.second); }

FrequencySmoother::FrequencySmoother(FrequencySmoother&& other) noexcept :
    accessTree(std::move(other.accessTree)),
    accessFreqs(std::move(other.accessFreqs))
{}


FrequencySmoother& FrequencySmoother::operator=(FrequencySmoother&& other) noexcept {
    if (this != &other) {
        accessTree = std::move(other.accessTree);
        accessFreqs = std::move(other.accessFreqs);
    }

    return *this;
}

FrequencySmoother::FrequencySmoother() : accessTree(freqCmp) {
    this->needTimeStamp=true;
}
FrequencySmoother::FrequencySmoother(bool needTimeStamp) : accessTree(freqCmp){
    this->needTimeStamp=needTimeStamp;
}

void FrequencySmoother::insert(std::string key) {
    {
        std::lock_guard<std::mutex> lock(m_mutex_);
//        if(accessFreqs.find(key)!=accessFreqs.end()) {
//            std::cout<<"WARNING: Key: "<<key<<" already exists"<<std::endl;
//            return;
//        }
        accessFreqs[key] = 0;
        accessTree.insert({key, 0});
    }
    if (needTimeStamp){
        std::lock_guard<std::mutex> lock(itemTimeStampMutex);
        std::string keyWithoutTimeStamp = key.substr(0, key.length() - 11);
        if(uniqueItemWithTimeStamp.find(keyWithoutTimeStamp) == uniqueItemWithTimeStamp.end()) {
            uniqueItemWithTimeStamp[keyWithoutTimeStamp] = std::set<long>();
        }
        uniqueItemWithTimeStamp[keyWithoutTimeStamp].insert(std::stol(key.substr(key.length() - 10)));
    }

    //print info
//    std::cout<< "Key: " << key << " is inserted" << std::endl;
}


int FrequencySmoother::getMinFrequency() {
	std::lock_guard<std::mutex> lock(m_mutex_);
	return accessTree.begin()->second;
}


std::string FrequencySmoother::getRUKey() {
    std::lock_guard<std::mutex> lock(m_mutex_);
    return accessTree.rbegin()->first;
}

std::string FrequencySmoother::getKeyWithMinFrequency() {
	std::lock_guard<std::mutex> lock(m_mutex_);
	return accessTree.begin()->first;
}
int FrequencySmoother::getKeyWithMinFrequencyRecordingAlpha() {
    std::lock_guard<std::mutex> lock(m_mutex_);
    return accessTree.begin()->second;
}
void FrequencySmoother::incrementFrequency(std::string key) {
	std::lock_guard<std::mutex> lock(m_mutex_);
	accessTree.erase({key, accessFreqs[key]});
	accessFreqs[key]++;
	accessTree.insert({key, accessFreqs[key]});
}

void FrequencySmoother::setFrequency(std::string key, int value) {
	std::lock_guard<std::mutex> lock(m_mutex_);
//    std::cout<< "Key: " << key << "'s old frequency: " << accessFreqs[key] << std::endl;
	accessTree.erase({key, accessFreqs[key]});
	accessFreqs[key] = value;
	accessTree.insert({key, accessFreqs[key]});
    //print info
//    std::cout<< "Key: " << key << "'s new frequency: " << accessFreqs[key] << std::endl;
}



void FrequencySmoother::removeKey(std::string key) {
    {
        std::lock_guard<std::mutex> lock(m_mutex_);
        accessTree.erase({key, accessFreqs[key]});
        accessFreqs.erase(key);
    }
    if(needTimeStamp){
        std::lock_guard<std::mutex> lock(itemTimeStampMutex);
        std::string keyWithoutTimeStamp = key.substr(0, key.length() - 11);
        if(uniqueItemWithTimeStamp.find(keyWithoutTimeStamp) != uniqueItemWithTimeStamp.end()) {
            uniqueItemWithTimeStamp[keyWithoutTimeStamp].erase(std::stol(key.substr(key.length() - 10)));
            if(uniqueItemWithTimeStamp[keyWithoutTimeStamp].empty()) {
                uniqueItemWithTimeStamp.erase(keyWithoutTimeStamp);
            }
        }

    }
}
void FrequencySmoother::removeKey_without_mutex(std::string key) {
    accessTree.erase({key, accessFreqs[key]});
    accessFreqs.erase(key);
    if(needTimeStamp){
        std::lock_guard<std::mutex> lock(itemTimeStampMutex);
        std::string keyWithoutTimeStamp = key.substr(0, key.length() - 11);
        if(uniqueItemWithTimeStamp.find(keyWithoutTimeStamp) != uniqueItemWithTimeStamp.end()) {
            uniqueItemWithTimeStamp[keyWithoutTimeStamp].erase(std::stol(key.substr(key.length() - 10)));
            if(uniqueItemWithTimeStamp[keyWithoutTimeStamp].empty()) {
                uniqueItemWithTimeStamp.erase(keyWithoutTimeStamp);
            }
        }

    }
}

void FrequencySmoother::addKey(std::string key) {
	std::lock_guard<std::mutex> lock(m_mutex_);
	accessTree.insert({key, accessFreqs[key]});
}

int FrequencySmoother::getFrequency(std::string key) {
	std::lock_guard<std::mutex> lock(m_mutex_);
	return accessFreqs[key];
}

int FrequencySmoother::size() {
	std::lock_guard<std::mutex> lock(m_mutex_);
	return accessFreqs.size();
}

std::set<std::pair<std::string, int>, decltype(&freqCmp)>::iterator FrequencySmoother::getIterator() {
	// std::lock_guard<std::mutex> lock(m_mutex_);
	return accessTree.begin();
}

std::mutex& FrequencySmoother::getMutex() {
	return m_mutex_;
}

void FrequencySmoother::search(const std::string pattern, std::vector<std::string> &results) {
    if (pattern.empty()) {
        fetchUniqueItemIDs(results);
        std::cout<<"Search all available keys: "<<results.size()<<std::endl;
        return;
    }
    {
        std::lock_guard<std::mutex> lock(itemTimeStampMutex);
        try {
            std::set<long> timeStamp =uniqueItemWithTimeStamp.at(pattern);
            for(auto it = timeStamp.begin(); it != timeStamp.end(); it++) {
                results.push_back(pattern + "@" + std::to_string(*it));
            }
        } catch (const std::out_of_range& e) {
            std::cout<<"No key found for pattern: "<<pattern<<std::endl;
            results.push_back("SEARCH FAILURE");
            return;
        }
    }
}

void FrequencySmoother::fetchUniqueItemIDs(std::vector<std::string> &results) {

    std::lock_guard<std::mutex> lock(itemTimeStampMutex);
    for(auto it = uniqueItemWithTimeStamp.begin(); it != uniqueItemWithTimeStamp.end(); it++) {
        results.push_back(it->first);
    }
}

bool FrequencySmoother::checkIfUniqueItemWithTimeStampExists(std::string &key) {
    //find key in accessFreqs
    std::lock_guard<std::mutex> lock(m_mutex_);
    return accessFreqs.find(key) != accessFreqs.end();
//    std::lock_guard<std::mutex> lock(itemTimeStampMutex);
//    std::string pattern = key.substr(0, key.length() - 11);
//    long timeStamp = std::stol(key.substr(key.length() - 10));
//    try {
//        std::set<long> timeStampSet = uniqueItemWithTimeStamp.at(pattern);
//        if (timeStampSet.find(timeStamp) != timeStampSet.end()) {
//            return true;
//        }
//        return false;
//    } catch (const std::out_of_range &e) {
//        return false;
//    }
}

std::string FrequencySmoother::getOldestKey(std::vector<std::string> keys_to_be_deleted) {
    long current_oldest_timestamp= std::numeric_limits<long>::max();
    std::string return_key="";
    {
        std::lock_guard<std::mutex> lock(itemTimeStampMutex);
        for (auto &it : uniqueItemWithTimeStamp) {
            for (auto &timestamp : it.second) {
                if (current_oldest_timestamp<= timestamp){
                    break;
                }
                std::string current_oldest_key = it.first + "@" + std::to_string(timestamp);
                if (std::find(keys_to_be_deleted.begin(), keys_to_be_deleted.end(), current_oldest_key) != keys_to_be_deleted.end()) {
                    continue;
                }
                current_oldest_timestamp = timestamp;
                return_key = current_oldest_key;
                if (current_oldest_timestamp==overall_oldest_timestamp){
                    return return_key;
                }
            }
        }
    }
    overall_oldest_timestamp=current_oldest_timestamp;
    std::cout<<"Overall oldest timestamp changed: "<<overall_oldest_timestamp<<std::endl;
    return return_key;
}