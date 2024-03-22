#include "FrequencySmoother.hpp"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <cassert>

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

FrequencySmoother::FrequencySmoother() : accessTree(freqCmp) {}

void FrequencySmoother::insert(std::string key) {
	std::lock_guard<std::mutex> lock(m_mutex_);
	if(accessFreqs.find(key)!=accessFreqs.end()) {
		return;
	}
	accessFreqs[key] = 0;
	accessTree.insert({key, 0});
    //print info
    std::cout<< "Key: " << key << " is inserted" << std::endl;
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

void FrequencySmoother::incrementFrequency(std::string key) {
	std::lock_guard<std::mutex> lock(m_mutex_);
	accessTree.erase({key, accessFreqs[key]});
	accessFreqs[key]++;
	accessTree.insert({key, accessFreqs[key]});
}

void FrequencySmoother::setFrequency(std::string key, int value) {
	std::lock_guard<std::mutex> lock(m_mutex_);
    std::cout<< "Key: " << key << "'s old frequency: " << accessFreqs[key] << std::endl;
	accessTree.erase({key, accessFreqs[key]});
	accessFreqs[key] = value;
	accessTree.insert({key, accessFreqs[key]});
    //print info
    std::cout<< "Key: " << key << "'s new frequency: " << accessFreqs[key] << std::endl;
}

void FrequencySmoother::removeKey(std::string key) {
	std::lock_guard<std::mutex> lock(m_mutex_);
    removeKey_without_mutex(key);
}
void FrequencySmoother::removeKey_without_mutex(std::string key) {
    accessTree.erase({key, accessFreqs[key]});
    accessFreqs.erase(key);
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
        return;
    }
//    std::lock_guard<std::mutex> lock(m_mutex_);

//    std::vector<std::string> tempResults;
    long oldest_timeStamp;
    int interval=999999999;
    bool successfulIteration = false;
    while (!successfulIteration) {
        try {
            oldest_timeStamp=UNIX_TIMESTAMP::current_time();
            interval=999999999;
            // Attempt to iterate over the data structure
            for(auto it = accessTree.begin(); it != accessTree.end(); it++) {
                if(it->first.find(pattern) != std::string::npos) {
                    long timeStamp=std::stol(it->first.substr(it->first.length()-10));
                    if(abs(timeStamp-oldest_timeStamp)<interval){
                        interval=abs(timeStamp-oldest_timeStamp);
                        assert (interval>=0);
                    }
                    if(timeStamp<oldest_timeStamp){
                        oldest_timeStamp=timeStamp;
                    }
                }
            }

            successfulIteration = true; // If we reach here, iteration was successful
        } catch (std::exception &e) {
            // Handle any exceptions, potentially indicating modifications during iteration
            // Decide on a policy here: delay before retrying, limit the number of retries, etc.
            std::cerr << "Standard exception caught: " << e.what() << std::endl;
            successfulIteration = false;
        } catch (...) {
            // Handle any other exceptions
            // Decide on a policy here: delay before retrying, limit the number of retries, etc.
            std::cerr << "Unknown exception caught" << std::endl;
            successfulIteration = false;
        }
    }

    results = std::vector<std::string>();
    std::cout<< "oldest_timeStamp: " << oldest_timeStamp << std::endl;
    std::cout<< "interval: " << interval << std::endl;
    results.push_back(std::to_string(oldest_timeStamp));
    results.push_back(std::to_string(interval));

}

void FrequencySmoother::fetchUniqueItemIDs(std::vector<std::string> &results) {
    std::lock_guard<std::mutex> lock(m_mutex_);
    std::set<std::string> uniqueSubstrings;

    for(auto it = accessTree.begin(); it != accessTree.end(); it++) {

        uniqueSubstrings.insert(it->first.substr(0, it->first.length() - 11));
    }
    std::vector<std::string> uniqueSubstringsVector(uniqueSubstrings.begin(), uniqueSubstrings.end());
    results = uniqueSubstringsVector;
}

bool FrequencySmoother::checkIfUniqueItemWithTimeStampExists(std::string &key) {
    bool successfulIteration = false;
    while (!successfulIteration) {
        try {
            for(auto it = accessTree.begin(); it != accessTree.end(); it++) {
                if(it->first == key) {
                    return true;
                }
            }

            successfulIteration = true; // If we reach here, iteration was successful
        } catch (std::exception &e) {
            // Handle any exceptions, potentially indicating modifications during iteration
            // Decide on a policy here: delay before retrying, limit the number of retries, etc.
            std::cerr << "Standard exception caught: " << e.what() << std::endl;
            successfulIteration = false;
        } catch (...) {
            // Handle any other exceptions
            // Decide on a policy here: delay before retrying, limit the number of retries, etc.
            std::cerr << "Unknown exception caught" << std::endl;
            successfulIteration = false;
        }
    }
    return false;
}
// void FrequencySmoother::storeFreq(std::string key, int freq) {
// 	std::lock_guard<std::mutex> lock(m_mutex_freq);
// 	freqStore[key] = freq;
// }

// int FrequencySmoother::getstoredFreq(std::string key) {
// 	std::lock_guard<std::mutex> lock(m_mutex_freq);
// 	return freqStore[key];
// }

// int FrequencySmoother::removestoredFreq(std::string key) {
// 	std::lock_guard<std::mutex> lock(m_mutex_freq);
// 	auto val  = freqStore[key];
// 	freqStore.erase(key);
// 	return val;
// }