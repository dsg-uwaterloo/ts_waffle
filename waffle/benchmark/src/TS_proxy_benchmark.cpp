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
                   int object_size, TimeSeriesDataMap& timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client, int discrepancy_limit) {
    std::string dummy(object_size, '0');
    int ops = 0, counter=0;
    int discrepancy;

    ops = client.num_requests_satisfied();
    uint64_t start, end, ckp1,ckp2;
    //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;
    auto ticks_per_ns = static_cast<double>(rdtscuhz()) / 1000;
    auto s = std::chrono::high_resolution_clock::now();
    auto e = std::chrono::high_resolution_clock::now();
    int elapsed = 0;
    std::vector<std::string> results;
    int i = 0;
    while (elapsed < run_time*1000000) {

        auto keys_values_pair = timeSeriesDataMap.generate_batch_TS_data();

//        if (stats) {
//            rdtscll(ckp1);
//        }
        while(true){
            discrepancy=counter-client.num_requests_satisfied()+ops;
            if(discrepancy<discrepancy_limit){
//                if(rand()%100==0)
//                    std::cout<<"Discrepancy "<<discrepancy;
                break;
            }
            else {
                std::cout<<"Put Discrepancy is "<<discrepancy<<"; counter = "<<counter<<"; finished = "<<client.num_requests_satisfied()-ops<<std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
//        if (stats) {
//            rdtscll(ckp2);
//        }
        //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;

        if (stats) {
            rdtscll(start);
        }
        client.put_batch(keys_values_pair.first, keys_values_pair.second);
        if (stats) {
            rdtscll(end);
            double cycles = static_cast<double>(end - start);
            latencies.push_back((cycles / ticks_per_ns) / client_batch_size);
            //analysis ckp1 and ckp2 time usage in persentage of the overall time usage.

//            if(rand()%100==0)
//                std::cout<<"CKP1: "<<(ckp1-start)/cycles<<"; CKP2: "<<(ckp2-start)/cycles<<std::endl;
//            rdtscll(start);
            //ops += keys_values_pair.first.size();
        }
        counter+=client_batch_size;

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
                         int object_size, std::vector<std::string>& available_keys, std::atomic<int> &xput, async_proxy_client client, async_proxy_client client_search,int discrepancy_limit=5000) {
    int ops = 0;
    ops = client.num_requests_satisfied();
    if (stats) {
        std::cout << "Before start Ops is " << ops << " client num_requests_satisfied is " << client.num_requests_satisfied() << std::endl;
    }
    int counter=0;

    int discrepancy;
    int64_t start, end, ckp1,ckp2,ckp3,ckp4;
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
            rdtscll(ckp1);
        }
        //choose a random key in available_keys
        std::string key = available_keys[index % available_keys.size()];
        index++;
        std::vector<std::string> keyWithTimeStamptsList;
        if (stats) {
            rdtscll(ckp2);
        }
//        std::cout<<"Going to search available time stamps for key "<<key<<std::endl;
        client_search.search(key,keyWithTimeStamptsList);
        if(keyWithTimeStamptsList[0]=="SEARCH FAILURE"){
            std::cout<<"No time stamps available for key: "<<key<<std::endl;
            client_search.search("",available_keys);
            continue;
        }
        if (stats) {
            rdtscll(ckp3);
        }
        if (rand()%100==0){
            std::cout<<"Number of Available time stamps for key "<<key<<" is "<<keyWithTimeStamptsList.size()<<std::endl;
        }
        //generate a random int between 0 and keyWithTimeStamptsList.size()-1 (inclusive)
        int randomIndex = rand() % keyWithTimeStamptsList.size();
        //set the keyWithTimeStampsList only contains the index greater or equal to randomIndex
        keyWithTimeStamptsList.erase(keyWithTimeStamptsList.begin(),keyWithTimeStamptsList.begin()+randomIndex);
        if (stats) {
            rdtscll(ckp4);
        }
        while(true){
            discrepancy=counter-client.num_requests_satisfied()+ops;
            if(discrepancy<discrepancy_limit){

                break;
            }
            else {
                std::cout<<"Query Discrepancy is "<<discrepancy<<"; counter = "<<counter<<"; finished = "<<client.num_requests_satisfied()-ops<<std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        if (stats) {
            rdtscll(start);
        }
        int num_batch=0;
//        std::cout<<"Number of Available time stamps for key "<<key<<" is "<<keyWithTimeStamptsList.size()<<std::endl;
        for (std::string keyWithTimeStamp: keyWithTimeStamptsList) {
            key_list.push_back(keyWithTimeStamp);
            counter++;
            if (key_list.size() == client_batch_size) {
                num_batch++;
                client.get_batch(key_list);
                key_list=std::vector<std::string>();
//                std::cout<<"Sleeping for 3 seconds"<<std::endl;f
            }
        }

        //get all the values for the timeStamps
        //std::cout << "Entering proxy_benchmark.cpp line " << __LINE__ << std::endl;
        if (stats and num_batch>0) {
            rdtscll(end);
            double cycles = static_cast<double>(end - start);
            for (int j = 0; j < num_batch; ++j)
                latencies.push_back((cycles / ticks_per_ns) / client_batch_size/num_batch);

            if(rand()%1000==0)
                std::cout<<"CKP1: "<<(ckp1-start)/cycles<<"; CKP2: "<<(ckp2-start)/cycles<<"CKP3: "<<(ckp3-start)/cycles<<"; CKP4: "<<(ckp4-start)/cycles<<std::endl;
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
            int object_size, TimeSeriesDataMap &timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client, int discrepancy_limit) {
    run_benchmark(15, false, latencies, client_batch_size, object_size, timeSeriesDataMap, xput, client, discrepancy_limit);
}

void cooldown(std::vector<int> &latencies, int client_batch_size,
              int object_size, TimeSeriesDataMap &timeSeriesDataMap, std::atomic<int> &xput, async_proxy_client client, int discrepancy_limit) {
    run_benchmark(15, false, latencies, client_batch_size, object_size, timeSeriesDataMap, xput, client, discrepancy_limit);
}

void client(int idx, int client_batch_size, int object_size, std::string &output_directory, std::string &host, int proxy_port, std::atomic<int> &xput,int no_items=10,double generation_interval=1, int discrepancy_limit=5000) {
    async_proxy_client client;
    idx=1;
    std::cout<<"Add a put client "<<idx<<std::endl;

    client.init(host, proxy_port);
    //generate keys
    //auto keys=ItemIdGenerator::generate_item_ids(no_items);
    std::vector<std::string> keys;
    ItemIdGenerator::read_item_ids(keys, "tracefiles/TS_ItemID_10000.txt", no_items);
    auto timeSeriesDataMap = TimeSeriesDataMap(keys, generation_interval, object_size,client_batch_size);

    // std::cout << "Client initialized with batch size " << client_batch_size << std::endl;
    std::atomic<int> indiv_xput;
    std::atomic_init(&indiv_xput, 0);
    std::vector<int> latencies;
    // std::cout << "Beginning warmup" << std::endl;
//    warmup(latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client);
    // std::cout << "Beginning benchmark" << std::endl;
    run_benchmark(30, true, latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client,discrepancy_limit);
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
    std::cout<<"PUT Overall Latency is: "<<latency_sum/latencies.size()<<std::endl;
    line.append("Xput: " + std::to_string(indiv_xput) + "\n");
    out << line;
    xput += indiv_xput;

    // std::cout << "xput is: " << xput << std::endl;
    // std::cout << "Beginning cooldown" << std::endl;
//    cooldown(latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client);

    client.finish();
}

void client_query(int idx, int client_batch_size, int object_size, std::string &output_directory, std::string &host, int proxy_port, std::atomic<int> &xput, int discrepancy=5000) {
    async_proxy_client client,client_search;

    idx=2;
    std::cout<<"Add a get client "<<idx<<std::endl;

    client.init(host, proxy_port);
    client_search.init(host, proxy_port);
    std::vector<std::string> available_keys;

    // std::cout << "Client initialized with batch size " << client_batch_size << std::endl;
    std::atomic<int> indiv_xput;
    std::atomic_init(&indiv_xput, 0);
    std::vector<int> latencies;
    // std::cout << "Beginning warmup" << std::endl;
//    warmup(latencies, client_batch_size, object_size, timeSeriesDataMap, indiv_xput, client);
//    run_benchmark_query(15, false, latencies, client_batch_size, object_size, available_keys, indiv_xput, client,client_search);

    // std::cout << "Beginning benchmark" << std::endl;
    run_benchmark_query(30, true, latencies, client_batch_size, object_size, available_keys, indiv_xput, client,client_search,discrepancy);
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
    std::cout<<"GET Overall Latency is: "<<latency_sum<<"  "<<latencies.size()<<" "<<latency_sum/latencies.size()<<std::endl;
    line.append("Xput: " + std::to_string(indiv_xput) + "\n");
    out << line;
    xput += indiv_xput;

    // std::cout << "xput is: " << xput << std::endl;
    // std::cout << "Beginning cooldown" << std::endl;
//    run_benchmark_query(15, false, latencies, client_batch_size, object_size, available_keys, indiv_xput, client,client_search);

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
    int discrepancy=5000;
    std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto date_string = std::string(std::ctime(&end_time));
    date_string = date_string.substr(0, date_string.rfind(":"));
    date_string.erase(remove(date_string.begin(), date_string.end(), ' '), date_string.end());
    std::string output_directory = "data/"+date_string;

    int o;
    std::string proxy_type_ = "pancake";
    while ((o = getopt(argc, argv, "b:h:p:t:s:b:n:o:i:g:d:")) != -1) {
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
            case'd':
                discrepancy=std::atoi(optarg);
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
    std::atomic<int> w_xput;
    std::atomic_init(&w_xput, 0);
    std::atomic<int> r_xput;
    std::atomic_init(&r_xput, 0);
    // std::cout << "trace loaded" << std::endl;

    std::vector<std::thread> threads;
    if (num_clients>0){
        for (int i = 0; i < std::max(num_clients/2,1); i++) {
            threads.push_back(std::thread(client, std::ref(i), std::ref(client_batch_size), std::ref(object_size),
                                          std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(w_xput),no_items,generation_interval, discrepancy));
        }
        for (int i = std::max(num_clients/2,1); i < num_clients; i++) {
            threads.push_back(std::thread(client_query, std::ref(i), std::ref(client_batch_size), std::ref(object_size),
                                          std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(r_xput),discrepancy));
        }
        for (int i = 0; i < num_clients; i++)
            threads[i].join();
    }else{
        for (int i = 0; i < std::max(-num_clients,1); i++) {
            threads.push_back(std::thread(client_query, std::ref(i), std::ref(client_batch_size), std::ref(object_size),
                                          std::ref(output_directory), std::ref(proxy_host), std::ref(proxy_port), std::ref(r_xput),discrepancy));
        }
        for (int i = 0; i < std::max(-num_clients,1); i++)
            threads[i].join();
    }

    std::cout << "write xput:"<<w_xput << std::endl;
    std::cout << "read xput:"<<r_xput << std::endl;
}