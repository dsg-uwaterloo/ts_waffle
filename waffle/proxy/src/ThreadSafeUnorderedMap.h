#include <unordered_map>
#include <mutex>
#include <memory>
#include <set>
#include "TS_value_master.h"

template<typename T>
class ThreadSafeUnorderedMap {
public:
    ThreadSafeUnorderedMap() = default;
    ThreadSafeUnorderedMap(const ThreadSafeUnorderedMap&) = delete;
    ThreadSafeUnorderedMap& operator=(const ThreadSafeUnorderedMap&) = delete;

    // Insert anyways but return True if key is already present
    bool insertIfNotPresent(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex_);
        bool isPresent = false;
        if(m_map_.find(key) != m_map_.end()) {
            isPresent = true;
        }
        if(get_map.find(key) != get_map.end()) {
            isPresent = true;
        }
        m_map_[key].push_back(nullptr);
        return isPresent;
    }

    bool insertIfNotPresent(const std::string& key, const std::shared_ptr<T>& value, const int request_id) {
        std::lock_guard<std::mutex> lock(m_mutex_);
        bool isPresent = false;
        // check m_map_
        if(m_map_.find(key) != m_map_.end()) {
            isPresent = true;
        }
        if (get_map.find(key) != get_map.end()) {
            isPresent = true;
        }
        get_map[key].insert(request_id);
        request_map[request_id].first.push_back(key);
        request_map[request_id].second = value;
        return isPresent;
    }

    void insert_cache(const int request_id, const std::string& key, const std::string& value, const std::shared_ptr<T>& p) {
        std::lock_guard<std::mutex> lock(m_mutex_);
        if (get_cache.find(key) != get_cache.end()) {
            if (get_cache[key] != value) {
                std::cerr << "Overwriting key in get cache: is this expected?" << std::endl;
                exit(1);
            }
        }
        get_cache[key] = value;

        if (request_map.find(request_id) == request_map.end()) {
            request_map[request_id].second = p;
        }
        request_map[request_id].first.push_back(key);
    }

    void tryClearPromises(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(m_mutex_);
        // erase it from m_map_
        m_map_.erase(key);
        // erase it from get_map, and try to clear the promise related to the key
        if(get_map.find(key) != get_map.end()) {
            get_cache[key] = value;
            for (int request_id : get_map[key])
            {
                if (isFulfilled(request_id)) {
                    clearPromises(request_id);
                    // clear the request_map
                    request_map.erase(request_id);
                    if (request_map.empty())
                    {
                        get_map.clear();
                        get_cache.clear();
                        break;
                    }
                }
            }
        }
    }

    void tryClearAllPromises() {
        std::lock_guard<std::mutex> lock(m_mutex_);
        std::vector<int> to_erase;
        for (auto request : request_map) {
            if (isFulfilled(request.first)) {
                to_erase.push_back(request.first);
                clearPromises(request.first);
            }
        }
        for (int request_id : to_erase) {
            request_map.erase(request_id);
        }
        if (request_map.empty()){
            get_map.clear();
            get_cache.clear();
        }
    }

    std::mutex& getMutex() {
	    return m_mutex_;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(m_mutex_);
        return m_map_.size();
    }
private:
    mutable std::mutex m_mutex_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<T>>> m_map_;
    // Below two maps are used only for processing user GET queries.
    // get_map is a map from key to multiple request_ids
    std::unordered_map<std::string, std::set<int>> get_map;
    // request_map is a map from request_id to a vector of keys it needs to fetch, and the promise it needs to resolve
    // because user GET request (request_id) is split into multiple database request (keys), but need to be returned in one response (promise)
    std::unordered_map<int, std::pair<std::vector<std::string>, std::shared_ptr<T>>> request_map;
    // The get_cache stores the <key, value> pair of partially fulfilled GET requests
    std::unordered_map<std::string, std::string> get_cache;

    bool isFulfilled(int request_id) {
        for (const std::string& key : request_map[request_id].first) {
            if (get_cache.find(key) == get_cache.end()) {
                return false;
            }
        }
        return true;
    }

    int deserializeAndSumInt(int request_id)
    {
        int total = 0;
        for (auto& key : request_map[request_id].first) {
            auto deserializedData = BinarySerializer::deserialize<int>(get_cache[key]);
            for (auto dp : deserializedData){
                total += dp;
            }
        }
        return total;
    }

    float deserializeAndSumFloat(int request_id)
    {
        float total = 0;
        for (auto &key : request_map[request_id].first)
        {
            auto deserializedData = BinarySerializer::deserialize<float>(get_cache[key]);
            for (auto dp : deserializedData)
            {
                total += dp;
            }
        }
        return total;
    }

    int deserializeAndSumBool(int request_id)
    {
        int total = 0;
        for (auto &key : request_map[request_id].first)
        {
            auto deserializedData = BinarySerializer::deserialize(get_cache[key]);
            for (auto dp : deserializedData)
            {
                total += dp;
            }
        }
        return total;
    }

    void clearPromises(const int request_id)
    {
        // deserialize all the values and resolve the promise
        // by default compute the sum of all values
        std::string data_type = DataType::get_data_type(request_map[request_id].first[0]);
        if (data_type == "int") {
            auto total = deserializeAndSumInt(request_id);
            request_map[request_id].second->set_value(std::to_string(total));
        }
        else if (data_type == "float") {
            auto total = deserializeAndSumFloat(request_id);
            request_map[request_id].second->set_value(std::to_string(total));
        }
        else {
            auto total = deserializeAndSumBool(request_id);
            request_map[request_id].second->set_value(std::to_string(total));
        }
    }
};
