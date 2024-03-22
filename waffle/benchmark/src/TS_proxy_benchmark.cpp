#ifdef _WIN32
#include <direct.h>
#endif

#include <unordered_map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include "timer.h"
#include "waffle_proxy.h"
#include "thrift_server.h"
#include "proxy_client.h"
#include "async_proxy_client.h"
#include "thrift_utils.h"
#include "TS_value_master.h"
#include "TS_key_master.h"
typedef std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>> trace_vector;


void run_benchmark(int run_time, bool stats, std::vector<int> &latencies, int client_batch_size,
                   int object_size, TimeSeriesDataMap& timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client) {
    std::string dummy(object_size, '0');
    int ops = 0;
    if (stats) {
        ops = client.num_requests_satisfied();
    }
    uint64_t start, end;
    //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;
    auto ticks_per_ns = static_cast<double>(rdtscuhz()) / 1000;
    auto s = std::chrono::high_resolution_clock::now();
    auto e = std::chrono::high_resolution_clock::now();
    int elapsed = 0;
    std::vector<std::string> results;
    int i = 0;
    while (elapsed < run_time*1000000) {
        if (stats) {
            rdtscll(start);
        }
        auto keys_values_pair = timeSeriesDataMap.generate_batch_TS_data();

        //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;
        client.put_batch(keys_values_pair.first, keys_values_pair.second);
        if (stats) {
            rdtscll(end);
            double cycles = static_cast<double>(end - start);
            latencies.push_back((cycles / ticks_per_ns) / client_batch_size);
            rdtscll(start);
            //ops += keys_values_pair.first.size();
        }
        e = std::chrono::high_resolution_clock::now();
        elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(e - s).count());
        ++i;
    }
    if (stats)
        ops = client.num_requests_satisfied() - ops;
    // std::cout << "Ops is " << ops << " client num_requests_satisfied is " << client.num_requests_satisfied() << std::endl;
    e = std::chrono::high_resolution_clock::now();
    elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(e - s).count());
    if (stats)
        xput += (int)(static_cast<double>(ops) * 1000000 / elapsed);
}

void run_benchmark_query(int run_time, bool stats, std::vector<int> &latencies, int client_batch_size,
                   int object_size, std::vector<std::string>& available_keys, std::atomic<int> &xput, async_proxy_client client) {
    int ops = 0;
    if (stats) {
        ops = client.num_requests_satisfied();
        std::cout << "Before start Ops is " << ops << " client num_requests_satisfied is " << client.num_requests_satisfied() << std::endl;
    }
    uint64_t start, end;
    //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;
    auto ticks_per_ns = static_cast<double>(rdtscuhz()) / 1000;
    auto s = std::chrono::high_resolution_clock::now();
    auto e = std::chrono::high_resolution_clock::now();
    int elapsed = 0;
    std::vector<std::string> results;
    int i = 0;
    client.search("",available_keys);
    while (elapsed < run_time*1000000) {
        if (stats) {
            rdtscll(start);
        }
        //choose a random key in available_keys
        std::string key = available_keys[rand() % available_keys.size()];
        std::vector<std::string> oldest_timeStamp_vector;
        client.search(key,oldest_timeStamp_vector);
        long oldest_timeStamp = std::stol(oldest_timeStamp_vector[0]);
        int timeInterval = std::stoi(oldest_timeStamp_vector[1]);
        long current_time=UNIX_TIMESTAMP::current_time();
        //generate all time stamps between oldest_timeStamp and current_time
        std::vector<std::string> key_list;
        for (long i = oldest_timeStamp; i < current_time; i+=timeInterval) {
            key_list.push_back(key+"@"+std::to_string(i));
        }
        client.get_batch(key_list);
        //get all the values for the timeStamps
        //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;
        if (stats) {
            rdtscll(end);
            double cycles = static_cast<double>(end - start);
            latencies.push_back((cycles / ticks_per_ns) / client_batch_size);
            rdtscll(start);
            //ops += keys_values_pair.first.size();
        }
        e = std::chrono::high_resolution_clock::now();
        elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(e - s).count());
        ++i;
    }
    if (stats)
    {
        ops = client.num_requests_satisfied() - ops;
        std::cout << "Ops is " << ops << " client num_requests_satisfied is " << client.num_requests_satisfied() << std::endl;
    }
    e = std::chrono::high_resolution_clock::now();
    elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(e - s).count());
    if (stats)
        xput += (int)(static_cast<double>(ops) * 1000000 / elapsed);
}
void warmup(std::vector<int> &latencies, int client_batch_size,
            int object_size, TimeSeriesDataMap &timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client) {
    run_benchmark(15, false, latencies, client_batch_size, object_size, timeSeriesDataMap, xput, client);
}

void cooldown(std::vector<int> &latencies, int client_batch_size,
              int object_size, TimeSeriesDataMap &timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client) {
    run_benchmark(15, false, latencies, client_batch_size, object_size, timeSeriesDataMap, xput, client);
}

void client(int idx, int client_batch_size, int object_size, std::string &output_directory, std::string &host, int proxy_port, std::atomic<int> &xput) {
    async_proxy_client client;
    client.init(host, proxy_port);


    //generate keys
    auto keys=ItemIdGenerator::generate_item_ids(1000000);
    auto timeSeriesDataMap = TimeSeriesDataMap(keys, 1, object_size,client_batch_size);

    // std::cout << "Client initialized with batch size " << client_batch_size << std::endl;
    std::atomic<int> indiv_xput;
    std::atomic_init(&indiv_xput, 0);
    std::vector<int> latencies;
    // std::cout << "Beginning warmup" << std::endl;
    warmup(latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client);
    // std::cout << "Beginning benchmark" << std::endl;
    run_benchmark(30, true, latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client);
    std::string location = output_directory + "/" + std::to_string(idx);
    std::ofstream out(location);
    std::string line("");
    for (auto lat : latencies) {
        line.append(std::to_string(lat) + "\n");
        out << line;
        line.clear();
    }
    line.append("Xput: " + std::to_string(indiv_xput) + "\n");
    out << line;
    xput += indiv_xput;

    // std::cout << "xput is: " << xput << std::endl;
    // std::cout << "Beginning cooldown" << std::endl;
    cooldown(latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client);

    client.finish();
}

void client_query(int idx, int client_batch_size, int object_size, std::string &output_directory, std::string &host, int proxy_port, std::atomic<int> &xput) {
    async_proxy_client client;
    client.init(host, proxy_port);


    std::vector<std::string> available_keys;

    // std::cout << "Client initialized with batch size " << client_batch_size << std::endl;
    std::atomic<int> indiv_xput;
    std::atomic_init(&indiv_xput, 0);
    std::vector<int> latencies;
    // std::cout << "Beginning warmup" << std::endl;
//    warmup(latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client);
    run_benchmark_query(15, true, latencies, client_batch_size, object_size, available_keys, indiv_xput, client);

    // std::cout << "Beginning benchmark" << std::endl;
    run_benchmark_query(30, true, latencies, client_batch_size, object_size, available_keys, indiv_xput, client);
    std::string location = output_directory + "/" + std::to_string(idx);
    std::ofstream out(location);
    std::string line("");
    for (auto lat : latencies) {
        line.append(std::to_string(lat) + "\n");
        out << line;
        line.clear();
    }
    line.append("Xput: " + std::to_string(indiv_xput) + "\n");
    out << line;
    xput += indiv_xput;

    // std::cout << "xput is: " << xput << std::endl;
    // std::cout << "Beginning cooldown" << std::endl;
    run_benchmark_query(15, true, latencies, client_batch_size, object_size, available_keys, indiv_xput, client);

    client.finish();
}
void usage() {
    std::cout << "Proxy client\n";
    std::cout << "\t -h: Proxy host name\n";
    std::cout << "\t -p: Proxy port\n";
    std::cout << "\t -t: Trace Location\n";
    std::cout << "\t -n: Number of threads to spawn\n";
    std::cout << "\t -s: Object Size\n";
    std::cout << "\t -o: Output Directory\n";
};

int _mkdir(const char *path) {
#ifdef _WIN32
    return ::_mkdir(path);
#else
#if _POSIX_C_SOURCE
    return ::mkdir(path, 0755);
#else
    return ::mkdir(path, 0755); // not sure if this works on mac
#endif
#endif
}

int main(int argc, char *argv[]) {
    std::string proxy_host = "192.168.252.109";
    int proxy_port = 9090;
    int client_batch_size = 50;
    int object_size = 1024;
    int num_clients = 10;

    std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto date_string = std::string(std::ctime(&end_time));
    date_string = date_string.substr(0, date_string.rfind(":"));
    date_string.erase(remove(date_string.begin(), date_string.end(), ' '), date_string.end());
    std::string output_directory = "data/"+date_string;

    int o;
    std::string proxy_type_ = "pancake";
    while ((o = getopt(argc, argv, "h:p:t:s:b:n:o:")) != -1) {
        switch (o) {
            case 'h':
                proxy_host = std::string(optarg);
                break;
            case 'p':
                proxy_port = std::atoi(optarg);
                break;
            case 's':
                object_size = std::atoi(optarg);
                break;
            case 'n':
                num_clients = std::atoi(optarg);
                break;
            case 'o':
                output_directory = std::string(optarg);
                break;
            default:
                usage();
                exit(-1);
        }
    }

    _mkdir((output_directory).c_str());
    std::atomic<int> w_xput;
    std::atomic_init(&w_xput, 0);
    std::atomic<int> r_xput;
    std::atomic_init(&r_xput, 0);
    // std::cout << "trace loaded" << std::endl;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_clients; i++) {
        threads.push_back(std::thread(client, std::ref(i), std::ref(client_batch_size), std::ref(object_size),
                                      std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(w_xput)));
    }
    for (int i = num_clients; i < 2*num_clients; i++) {
        threads.push_back(std::thread(client_query, std::ref(i), std::ref(client_batch_size), std::ref(object_size),
                                      std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(r_xput)));
    }
    for (int i = 0; i < 2*num_clients; i++)
        threads[i].join();
    // std::cout << "Xput was: " << xput << std::endl;
    std::cout << "write xput:"<<w_xput << std::endl;
    std::cout << "read xput:"<<r_xput << std::endl;
}