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
#include <iostream>
#include <chrono>
#include <thread>

#include "tsconfig.h"

typedef std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>> trace_vector;

void load_trace(const std::string &trace_location, trace_vector &trace, int client_batch_size) {
    std::vector<std::string> put_keys;
    std::vector<std::string> put_values;
    std::vector<std::string> get_keys;

    TimeSeriesDataMap::DataPair data_pair = TimeSeriesDataMap::get_TS_datapair_from_file(trace_location);
    auto keys = data_pair[0];
    auto values = data_pair[1];
    int batch_count = client_batch_size;
    bool processing_put = true;
    for (int i = 0; i < keys.size(); i++)
    {
        if (values[i] == "") {
            // processing get
            if (get_keys.size() == batch_count) {
                trace.push_back(std::make_pair(get_keys, std::vector<std::string>()));
                get_keys.clear();
            }
            if (processing_put) {
                trace.push_back(std::make_pair(put_keys, put_values));
                put_keys.clear();
                put_values.clear();
                processing_put = false;
            }
            get_keys.push_back(keys[i]);
        }
        else {
            // processing put
            if (put_keys.size() == batch_count) {
                trace.push_back(std::make_pair(put_keys, put_values));
                put_keys.clear();
                put_values.clear();
            }
            if (!processing_put) {
                trace.push_back(std::make_pair(get_keys, std::vector<std::string>()));
                get_keys.clear();
                processing_put = true;
            }
            put_keys.push_back(keys[i]);
            put_values.push_back(values[i]);
        }
    }
    if (get_keys.size() > 0){
        trace.push_back(std::make_pair(get_keys, std::vector<std::string>()));
        get_keys.clear();
    }
    if (put_keys.size() > 0){
        trace.push_back(std::make_pair(put_keys, put_values));
        put_keys.clear();
        put_values.clear();
    }
}

void run_benchmark(int run_time, bool stats, std::vector<int> &latencies, std::atomic<int> &xput, async_proxy_client client, int discrepancy_limit, trace_vector &trace) {
    int ops = 0;

    ops = client.num_requests_satisfied();
    uint64_t start, end;
    //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;
    auto ticks_per_ns = static_cast<double>(rdtscuhz()) / 1000;
    auto s = std::chrono::high_resolution_clock::now();
    auto e = std::chrono::high_resolution_clock::now();
    int elapsed = 0;
    std::vector<std::string> results;
    int i = 0;
    while (elapsed < run_time*1000000) {
        if (i == trace.size()) {
            std::cout << "Trace size is " << trace.size() << std::endl;
            break;
        }
        auto keys_values_pair = trace[i];

        if (stats) {
            rdtscll(start);
        }

        if (keys_values_pair.second.empty()) {
            auto result = client.get_batch(keys_values_pair.first);
            // if (i < 30) {
            //     std::cout << "Get result is " << result << std::endl;
            // }
        }
        else {
            client.put_batch(keys_values_pair.first, keys_values_pair.second);
        }
        if (stats) {
            rdtscll(end);
            double cycles = static_cast<double>(end - start);
            latencies.push_back((cycles / ticks_per_ns) / benchmarkConfig::client_batch_size);
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
    std::cout<<"Time taken for benchmark (original time setting: "<<run_time<<") is "<<elapsed<<std::endl;
}

void warmup(std::vector<int> &latencies, std::atomic<int> &xput, async_proxy_client client, int discrepancy_limit, trace_vector &trace) {
    run_benchmark(15, false, latencies, xput, client, discrepancy_limit, trace);
}

void cooldown(std::vector<int> &latencies, std::atomic<int> &xput, async_proxy_client client, int discrepancy_limit, trace_vector &trace) {
    run_benchmark(15, false, latencies, xput, client, discrepancy_limit, trace);
}

void client(int idx, std::string &output_directory, std::string &host, int proxy_port, trace_vector &trace, std::atomic<int> &xput, int discrepancy_limit = 5000)
{
    async_proxy_client client;
    idx=1;

    client.init(host, proxy_port);

    std::atomic<int> indiv_xput;
    std::atomic_init(&indiv_xput, 0);
    std::vector<int> latencies;
    // std::cout << "Beginning warmup" << std::endl;
//    warmup(latencies, indiv_xput, client, trace);
    // std::cout << "Beginning benchmark" << std::endl;
    run_benchmark(30, true, latencies, indiv_xput, client,discrepancy_limit, trace);
    std::string location = output_directory + "/" + std::to_string(idx);
    std::ofstream out(location);
    std::string line("");
    double latency_sum=0;
    for (auto lat : latencies) {
        latency_sum+=lat;
        line.append(std::to_string(lat) + "\n");
        out << line;
        line.clear();
    }
    std::cout<<"Overall Latency is: "<<latency_sum/latencies.size()<<std::endl;
    line.append("Xput: " + std::to_string(indiv_xput) + "\n");
    out << line;
    xput += indiv_xput;

    // std::cout << "xput is: " << xput << std::endl;
    // std::cout << "Beginning cooldown" << std::endl;
//    cooldown(latencies, indiv_xput, client, trace);

    client.finish();
}
void usage() {
    std::cout << "Proxy client\n";
    std::cout << "\t -h: Proxy host name\n";
    std::cout << "\t -p: Proxy port\n";
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
    std::string trace_location = benchmarkConfig::trace_location;
    int num_clients = benchmarkConfig::num_clients;
    int discrepancy=5000;
    std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto date_string = std::string(std::ctime(&end_time));
    date_string = date_string.substr(0, date_string.rfind(":"));
    date_string.erase(remove(date_string.begin(), date_string.end(), ' '), date_string.end());
    std::string output_directory = "data/"+date_string;

    int o;
    std::string proxy_type_ = "pancake";
    while ((o = getopt(argc, argv, "h:p:")) != -1) {
        switch (o) {
            case 'h':
                proxy_host = std::string(optarg);
                break;
            case 'p':
                proxy_port = std::atoi(optarg);
                break;
            default:
                usage();
                exit(-1);
        }
    }

    _mkdir((output_directory).c_str());
    std::cout << "The current output directory is: " << output_directory << std::endl;
    {
        std::string fileName = "client_args.txt";
        std::string filePath = output_directory + "/" + fileName;
        std::ofstream outFile(filePath);
        if (!outFile) {
            std::cerr << "Failed to open " << filePath << " for writing." << std::endl;
        }
        for (int i = 1; i < argc; ++i) {
            outFile << argv[i] << std::endl;
        }
        outFile.close();
    }
    std::atomic<int> xput;
    std::atomic_init(&xput, 0);

    trace_vector trace;
    load_trace(trace_location, trace, benchmarkConfig::client_batch_size);
    // std::cout << "trace loaded" << std::endl;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_clients; i++) {
        threads.push_back(std::thread(client, std::ref(i), std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(trace), std::ref(xput), discrepancy));
    }
    for (int i = 0; i < num_clients; i++)
        threads[i].join();
    // std::cout << "Xput was: " << xput << std::endl;
    std::cout << xput << std::endl;
}
