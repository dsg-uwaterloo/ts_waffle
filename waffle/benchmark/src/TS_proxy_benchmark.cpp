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
typedef std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>> trace_vector;


void run_benchmark(int run_time, bool stats, std::vector<int> &latencies, int client_batch_size,
                   int object_size, TimeSeriesDataMap& timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client) {
    std::string dummy(object_size, '0');
    int ops = 0, counter=0;
    int discrepancy;

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
        if (stats) {
            rdtscll(start);
        }
        auto keys_values_pair = timeSeriesDataMap.generate_batch_TS_data();
        while(true){
            discrepancy=counter-client.num_requests_satisfied()+ops;
            if(discrepancy<2000){
                break;
            }
            else {
                std::cout<<"Put Discrepancy is "<<discrepancy<<"; counter = "<<counter<<"; finished = "<<client.num_requests_satisfied()-ops<<std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
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
    std::cout<<"Time taken for put benchmark (original time setting: "<<run_time<<") is "<<elapsed<<std::endl;
//    while(true){
//        discrepancy=counter-client.num_requests_satisfied()+ops;
//        if(discrepancy<100){
//            break;
//        }
//        else {
//            std::cout<<"Cool Down Put Discrepancy is "<<discrepancy<<"; counter = "<<counter<<"; finished = "<<client.num_requests_satisfied()-ops<<std::endl;
//            std::this_thread::sleep_for(std::chrono::milliseconds(100));
//        }
//    }
}

void run_benchmark_query(int run_time, bool stats, std::vector<int> &latencies, int client_batch_size,
                         int object_size, std::vector<std::string>& available_keys, std::atomic<int> &xput, async_proxy_client client, async_proxy_client client_search) {
    int ops = 0;
    ops = client.num_requests_satisfied();
    if (stats) {
        std::cout << "Before start Ops is " << ops << " client num_requests_satisfied is " << client.num_requests_satisfied() << std::endl;
    }
    int counter=0;

    int discrepancy;
    uint64_t start, end;
    //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;
    auto ticks_per_ns = static_cast<double>(rdtscuhz()) / 1000;
    auto s = std::chrono::high_resolution_clock::now();
    auto e = std::chrono::high_resolution_clock::now();
    int elapsed = 0;
    std::vector<std::string> results;
    int i = 0;
    client_search.search("",available_keys);
    //print size of available_keys
    std::cout << "Size of available_keys is " << available_keys.size() << std::endl;
    std::vector<std::string> key_list;
    int index= 0;
    while (elapsed < run_time*1000000) {
        if (stats) {
            rdtscll(start);
        }
        //choose a random key in available_keys
        std::string key = available_keys[index % available_keys.size()];
        index++;
        std::vector<std::string> keyWithTimeStamptsList;

//        std::cout<<"Going to search available time stamps for key "<<key<<std::endl;
        client_search.search(key,keyWithTimeStamptsList);
        if(keyWithTimeStamptsList[0]=="SEARCH FAILURE"){
            std::cout<<"No time stamps available for key: "<<key<<std::endl;
            client_search.search("",available_keys);
            continue;
        }
        if (rand()%10==0){
            std::cout<<"Number of Available time stamps for key "<<key<<" is "<<keyWithTimeStamptsList.size()<<std::endl;
        }
        //generate a random int between 0 and keyWithTimeStamptsList.size()-1 (inclusive)
        int randomIndex = rand() % keyWithTimeStamptsList.size();
        //set the keyWithTimeStampsList only contains the index greater or equal to randomIndex
        keyWithTimeStamptsList.erase(keyWithTimeStamptsList.begin(),keyWithTimeStamptsList.begin()+randomIndex);

//        std::cout<<"Number of Available time stamps for key "<<key<<" is "<<keyWithTimeStamptsList.size()<<std::endl;
        for (std::string keyWithTimeStamp: keyWithTimeStamptsList) {
            key_list.push_back(keyWithTimeStamp);
            counter++;
            if (key_list.size() == client_batch_size) {
//                std::cout<<"Going to get batch for keys: "<<std::endl;
//                for (auto key : key_list) {
//                    std::cout<<"\t"<<key<<std::endl;
//                }


                while(true){
                    discrepancy=counter-client.num_requests_satisfied()+ops;
                    if(discrepancy<2000){
                        break;
                    }
                    else {
                        std::cout<<"Query Discrepancy is "<<discrepancy<<"; counter = "<<counter<<"; finished = "<<client.num_requests_satisfied()-ops<<std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                        if(rand()%50==0){
//                            client_search.search("",available_keys);
//                        }
                    }
                }
                client.get_batch(key_list);
                key_list=std::vector<std::string>();
//                std::cout<<"Sleeping for 3 seconds"<<std::endl;f
            }
        }


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
    std::cout<<"Time taken for query benchmark (original time setting: "<<run_time<<") is "<<elapsed<<std::endl;
//    while(true){
//        discrepancy=counter-client.num_requests_satisfied()+ops;
//        if(discrepancy<100){
//            break;
//        }
//        else {
//            std::cout<<"Cool Down Query Discrepancy is "<<discrepancy<<"; counter = "<<counter<<"; finished = "<<client.num_requests_satisfied()-ops<<std::endl;
//            std::this_thread::sleep_for(std::chrono::milliseconds(100));
//        }
//    }
}
void warmup(std::vector<int> &latencies, int client_batch_size,
            int object_size, TimeSeriesDataMap &timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client) {
    run_benchmark(15, false, latencies, client_batch_size, object_size, timeSeriesDataMap, xput, client);
}

void cooldown(std::vector<int> &latencies, int client_batch_size,
              int object_size, TimeSeriesDataMap &timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client) {
    run_benchmark(15, false, latencies, client_batch_size, object_size, timeSeriesDataMap, xput, client);
}

void client(int idx, int client_batch_size, int object_size, std::string &output_directory, std::string &host, int proxy_port, std::atomic<int> &xput,int no_items=10,double generation_interval=1) {
    async_proxy_client client;
    client.init(host, proxy_port);


    //generate keys
//    auto keys=ItemIdGenerator::generate_item_ids(no_items);
    std::vector<std::string> keys;
    ItemIdGenerator::read_item_ids(keys, "tracefiles/TS_ItemID_10000.txt", no_items);
    auto timeSeriesDataMap = TimeSeriesDataMap(keys, generation_interval, object_size,client_batch_size);

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
    async_proxy_client client,client_search;
    client.init(host, proxy_port);
    client_search.init(host, proxy_port);


    std::vector<std::string> available_keys;

    // std::cout << "Client initialized with batch size " << client_batch_size << std::endl;
    std::atomic<int> indiv_xput;
    std::atomic_init(&indiv_xput, 0);
    std::vector<int> latencies;
    // std::cout << "Beginning warmup" << std::endl;
//    warmup(latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client);
    run_benchmark_query(15, false, latencies, client_batch_size, object_size, available_keys, indiv_xput, client,client_search);

    // std::cout << "Beginning benchmark" << std::endl;
    run_benchmark_query(30, true, latencies, client_batch_size, object_size, available_keys, indiv_xput, client,client_search);
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
    run_benchmark_query(15, false, latencies, client_batch_size, object_size, available_keys, indiv_xput, client,client_search);

    client.finish();
    client_search.finish();
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
    int num_clients = 2;
    int no_items=10;
    double generation_interval=0.00001;

    std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto date_string = std::string(std::ctime(&end_time));
    date_string = date_string.substr(0, date_string.rfind(":"));
    date_string.erase(remove(date_string.begin(), date_string.end(), ' '), date_string.end());
    std::string output_directory = "data/"+date_string;

    int o;
    std::string proxy_type_ = "pancake";
    while ((o = getopt(argc, argv, "b:h:p:t:s:b:n:o:i:g:")) != -1) {
        switch (o) {
            case 'b':
                client_batch_size = std::atoi(optarg);
                break;
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
            case'i':
                no_items=std::atoi(optarg);
                break;
            case'g':
                generation_interval=std::atof(optarg);
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
    for (int i = 0; i < std::max(num_clients/2,1); i++) {
        std::cout<<"Add a put client"<<std::endl;
        threads.push_back(std::thread(client, std::ref(i), std::ref(client_batch_size), std::ref(object_size),
                                      std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(w_xput),no_items,generation_interval));
    }
    for (int i = std::max(num_clients/2,1); i < num_clients; i++) {
        std::cout<<"Add a query client"<<std::endl;
        threads.push_back(std::thread(client_query, std::ref(i), std::ref(client_batch_size), std::ref(object_size),
                                      std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(r_xput)));
    }
//    for (int i = 0; i < num_clients; i++) {
//        threads.push_back(std::thread(client_query, std::ref(i), std::ref(client_batch_size), std::ref(object_size),
//                                      std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(r_xput)));
//    }

    for (int i = 0; i < num_clients; i++)
        threads[i].join();

    // std::cout << "Xput was: " << xput << std::endl;
    std::cout << "write xput:"<<w_xput << std::endl;
    std::cout << "read xput:"<<r_xput << std::endl;
}