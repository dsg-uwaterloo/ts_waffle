#ifndef FREQ_SMOOTHER_H
#define FREQ_SMOOTHER_H

#include <string>
#include <unordered_map>
#include <utility>
#include <set>
#include <mutex>
#include <memory>


bool freqCmp(std::pair<std::string, int> a, std::pair<std::string, int> b);

class FrequencySmoother {

private:
	// std::unordered_map<std::string, int> freqStore;

public:
    std::unordered_map<std::string, int> accessFreqs;

    std::set<std::pair<std::string, int>, decltype(&freqCmp)> accessTree;
	FrequencySmoother(FrequencySmoother&& other) noexcept;
	FrequencySmoother& operator=(FrequencySmoother&& other) noexcept;
	FrequencySmoother();
	void insert(std::string key);
	int getMinFrequency();
	void incrementFrequency(std::string key);
	void setFrequency(std::string key, int value);
	void removeKey(std::string key);
    void removeKey_without_mutex(std::string key);
	void addKey(std::string key);
	std::string getKeyWithMinFrequency();
    std::string getRUKey();
	int size();
	int getFrequency(std::string key);
	std::set<std::pair<std::string, int>, decltype(&freqCmp)>::iterator getIterator();
	std::mutex& getMutex();
	// void storeFreq(std::string key, int freq);
	// int getstoredFreq(std::string key);
	// int removestoredFreq(std::string key);
	mutable std::mutex m_mutex_;
	// mutable std::mutex m_mutex_freq;
};

#endif