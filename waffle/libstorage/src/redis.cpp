#include <stdexcept>
#include <iostream>
#include <future>
#include <assert.h>
#include "redis.h"

    redis::redis(const std::string &host_name, int port){
        std::cout << "Redis init() called" << std::endl;
        this->clients.push_back(std::move(std::make_shared<cpp_redis::client>()));
        this->clients.back()->connect(host_name, port,
                [](const std::string &host, std::size_t port, cpp_redis::client::connect_state status) {
                    if (status == cpp_redis::client::connect_state::dropped || status == cpp_redis::client::connect_state::failed || status == cpp_redis::client::connect_state::lookup_failed){
                        std::cerr << "Redis client disconnected from " << host << ":" << port << std::endl;
                        exit(-1);
                    }
                });
    }

    void redis::add_server(const std::string &host_name, int port){
        this->clients.push_back(std::move(std::make_shared<cpp_redis::client>()));
        this->clients.back()->connect(host_name, port,
                                  [](const std::string &host, std::size_t port, cpp_redis::client::connect_state status) {
                                      if (status == cpp_redis::client::connect_state::dropped || status == cpp_redis::client::connect_state::failed || status == cpp_redis::client::connect_state::lookup_failed){
                                          std::cerr << "Redis client disconnected from " << host << ":" << port << std::endl;
                                          exit(-1);
                                      }
                                  });
    }

    std::string redis::get(const std::string &key){
        auto idx = (std::hash<std::string>{}(std::string(key)) % clients.size());
        auto fut = clients[idx]->get(key);
        clients[idx]->commit();
        auto reply = fut.get();
        if (reply.is_error()){
            throw std::runtime_error(reply.error());
        }
        return reply.as_string();
    }

    void redis::put(const std::string &key, const std::string &value){
        auto idx = (std::hash<std::string>{}(std::string(key)) % clients.size());
        auto fut = clients[idx]->set(key, value);
        clients[idx]->commit();
        auto reply = fut.get();
        if (reply.is_error()){
            throw std::runtime_error(reply.error());
        }
    }

    std::vector<std::string> redis::get_batch(const std::vector<std::string> &keys){
        std::queue<std::future<cpp_redis::reply>> futures;
        std::unordered_map<int, std::vector<std::string>> key_vectors;

        // Gather all relevant storage interface's by id and create vector for key batch
//        int count=0;
        for (const auto &key: keys) {
//            count++;
//            if (!key_exists(key) )
//            {
//                std::cout << "Key at "<<count<<" does not exist: " << key << std::endl;
//                throw std::runtime_error("Key does not exist");
//            }
            auto id = (std::hash<std::string>{}(std::string(key)) % clients.size());
            key_vectors[id].emplace_back(key);
        }


        for (auto it = key_vectors.begin(); it != key_vectors.end(); it++) {
             // std::cout << "Entering redis.cpp line " << __LINE__ << std::endl;
            auto future = clients[it->first]->mget(it->second);
            futures.push(std::move(future));

        }
        // Issue requests to each storage server
        for (auto it = key_vectors.begin(); it != key_vectors.end(); it++)
            clients[it->first]->commit();
        std::vector< std::string> return_vector;

        for (int i = 0; i < futures.size(); i++) {
            auto reply = futures.front().get();
            futures.pop();
            if (reply.is_error()){
                throw std::runtime_error(reply.error());
            }
            auto reply_array = reply.as_array();
            for (auto nested_reply: reply_array){
                if (nested_reply.is_error()){
                    throw std::runtime_error(nested_reply.error());
                }
                return_vector.push_back(nested_reply.as_string());
            }
        }
//        std::cout<<"Redis Server Size (get): "<<get_database_size()<<std::endl;
        return return_vector;
    }

    void redis::put_batch(const std::vector<std::string> &keys, const std::vector<std::string> &values){
        std::queue<std::future<cpp_redis::reply>> futures;
        std::unordered_map<int, std::vector<std::pair<std::string, std::string>>> key_value_vector_pairs;

//         Gather all relevant storage interface's by id and create vector for key batch
        int i = 0;
//        int count_dulicate=0;
        for (const auto &key: keys) {
            //print if key exists
//            if (key_exists(key) )
//            {
//                count_dulicate++;
//            }

            auto id = (std::hash<std::string>{}(std::string(key)) % clients.size());
            key_value_vector_pairs[id].push_back(std::make_pair(key, values[i]));
            i++;
        }
//        std::cout << "Duplicate keys: " << count_dulicate << std::endl;
        for (auto it = key_value_vector_pairs.begin(); it != key_value_vector_pairs.end(); it++) {
            auto future = clients[it->first]->mset(it->second);
            futures.push(std::move(future));
        }

        // Issue requests to each storage server
        for (auto it = key_value_vector_pairs.begin(); it != key_value_vector_pairs.end(); it++)
            clients[it->first]->commit();

        std::shared_ptr<std::vector< std::string>> return_vector;

        for (int i = 0; i < futures.size(); i++){
            auto reply = futures.front().get();
            futures.pop();
            if (reply.is_error()){
                throw std::runtime_error(reply.error());
            }
        }
//        std::cout<<"Redis Server Size (put): "<<get_database_size()<<std::endl;
    }

    void redis::delete_batch(const std::vector<std::string> &keys) {
        std::unordered_map<int, std::vector<std::string>> key_vectors;
         // Gather all relevant storage interface's by id and create vector for key batch
        for (const auto &key: keys) {
            auto id = (std::hash<std::string>{}(std::string(key)) % clients.size());
            key_vectors[id].emplace_back(key);
        }

        for (auto it = key_vectors.begin(); it != key_vectors.end(); it++) {
             // std::cout << "Entering redis.cpp line " << __LINE__ << std::endl;
            clients[it->first]->del(it->second);
        }

        for (auto it = key_vectors.begin(); it != key_vectors.end(); it++)
            clients[it->first]->commit();

//        std::cout<<"Redis Server Size (delete): "<<get_database_size()<<std::endl;
    }
    size_t redis::get_database_size() {
        // Assuming you want to check the size of the first (or any specific) Redis instance.
        if (clients.empty()) {
            throw std::runtime_error("No Redis clients are connected.");
        }

        auto& client = clients.front(); // Get the first client or modify to select a specific one.
        auto future = client->dbsize();
        client->commit(); // Make sure to commit to send the command to the server.
        auto reply = future.get(); // Wait for and get the reply.

        if (reply.is_error()) {
            throw std::runtime_error(reply.error());
        }

        // dbsize command returns the number of keys as an integer.
        return reply.as_integer();
    }
bool redis::key_exists(const std::string &key) {
    // Choose the appropriate client based on the key's hash, similar to how you do in get or put.
    auto idx = (std::hash<std::string>{}(key) % clients.size());
    auto& client = clients[idx];

    // Use the exists command to check for the key.
    auto future = client->exists({key});
    client->commit(); // Ensure the command is sent to the server.
    auto reply = future.get(); // Wait for and retrieve the response.

    if (reply.is_error()) {
        // Handle error appropriately, possibly re-throw or return false.
        throw std::runtime_error(reply.error());
    }

    // The exists command returns the count of keys that exist.
    return reply.as_integer() > 0;
}
//
//void redis::key_list(const std::string &pattern, std::vector<std::string> &keys) {
//    // Choose the appropriate client based on the pattern, similar to how you do in get or put.
//    auto idx = (std::hash<std::string>{}(pattern) % clients.size());
//    auto& client = clients[idx];
//
//    // Use the keys command to retrieve the keys matching the pattern.
//    auto future = client->keys(pattern);
//    client->commit(); // Ensure the command is sent to the server.
//    auto reply = future.get(); // Wait for and retrieve the response.
//
//    if (reply.is_error()) {
//        // Handle error appropriately, possibly re-throw or return false.
//        throw std::runtime_error(reply.error());
//    }
//
//    // The keys command returns an array of keys matching the pattern.
//    auto key_array = reply.as_array();
//    for (auto& key : key_array) {
//        keys.push_back(key.as_string());
//    }
//}
