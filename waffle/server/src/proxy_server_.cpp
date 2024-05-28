#include <unordered_map>
#include <iostream>

#include "waffle_proxy.h"
//#include "thrift_response_client_map.h"
#include "thrift_server.h"
#include "thrift_utils.h"

#include "TS_value_master.h"
#include "TS_key_master.h"


#define HOST "127.0.0.1"
#define PROXY_PORT 9090

typedef std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>> trace_vector;

void getKeysValues(const std::string &trace_location, std::vector<std::string>& keys, std::vector<std::string>& values) {
    std::ifstream in_workload_file;
    in_workload_file.open(trace_location, std::ios::in);
    if(!in_workload_file.is_open()){
        std::perror("Unable to find workload file");
    }
    if(in_workload_file.fail()){
        std::perror("Opening workload file failed");
    }
    std::string line;
    std::string op, key, val;
    while (std::getline(in_workload_file, line)) {
        op = line.substr(0, line.find(" "));
        key = line.substr(line.find(" ")+1);
        val = "";

        if (key.find(" ") != -1) {
            val = key.substr(key.find(" ")+1);
            key = key.substr(0, key.find(" "));
        }

        keys.push_back(key);
        values.push_back(val);
        assert (key != "SET");
    }
    in_workload_file.close();
};

void flush_thread(std::shared_ptr<proxy> proxy){
    while (true){
        sleep(1);
        dynamic_cast<waffle_proxy&>(*proxy).flush();
    }
    std::cout << "Quitting flush thread" << std::endl;
}

void usage() {
    std::cout << "TS Waffle proxy:\n";
    // Network Parameters
    std::cout << "\t -h: Storage server host name\n";
    std::cout << "\t -p: Storage server port\n";
    std::cout << "\t -s: Storage server type (redis, rocksdb, memcached)\n";
    std::cout << "\t -a: Storage server count\n";
    std::cout << "\t -z: Proxy server type\n";
    // Workload parameters
    std::cout << "\t -l: Workload file\n";
    std::cout << "\t -v: Value size\n";
    std::cout << "\t -b: Security batch size\n";
    std::cout << "\t -c: Storage batch size\n";
    std::cout << "\t -t: Number of worker threads for cpp_redis\n";
    // Other parameters
    std::cout << "\t -o: Output location for sizing thread\n";
    std::cout << "\t -d: Core to run on\n";
};

int main(int argc, char *argv[]) {
    std::cout << std::boolalpha;
    int client_batch_size = 50;
    std::atomic<int> xput;
    std::atomic_init(&xput, 0);

    std::shared_ptr<proxy> proxy_ = std::make_shared<waffle_proxy>();
    int o;
    while ((o = getopt(argc, argv, "h:p:s:n:v:b:c:t:o:d:z:q:l:m:r:y:f:a")) != -1) {
        switch (o) {
            case 'h':
                dynamic_cast<waffle_proxy&>(*proxy_).server_host_name_ = std::string(optarg);
                break;
            case 'p':
                dynamic_cast<waffle_proxy&>(*proxy_).server_port_ = std::atoi(optarg);
                break;
            case 'a':
                dynamic_cast<waffle_proxy&>(*proxy_).num_cores = std::atoi(optarg);
                break;
            case 'c':
                dynamic_cast<waffle_proxy&>(*proxy_).cacheBatches = std::atoi(optarg);
                break;
            case 'z':
                dynamic_cast<waffle_proxy&>(*proxy_).security_batch_size_ = std::atoi(optarg);
                break;
            // case 'c':
            //     dynamic_cast<waffle_proxy&>(*proxy_).storage_batch_size_ = std::atoi(optarg);
            //     break;
            case 't':
                dynamic_cast<waffle_proxy&>(*proxy_).p_threads_ = std::atoi(optarg);
                break;
            case 'o':
                dynamic_cast<waffle_proxy&>(*proxy_).output_location_ = std::string(optarg);
                break;
            case 'q':
                client_batch_size = std::atoi(optarg);
                break;
//            case 'l':
//                dynamic_cast<waffle_proxy&>(*proxy_).trace_location_ = std::string(optarg);
//                break;
//                //print information
            case 'y':
                dynamic_cast<waffle_proxy&>(*proxy_).latency = true;
                break;
            case 'b':
                dynamic_cast<waffle_proxy&>(*proxy_).B = std::atoi(optarg);
                break;
            case 'r':
                dynamic_cast<waffle_proxy&>(*proxy_).R = std::atoi(optarg);
                break;
            case 'f':
                dynamic_cast<waffle_proxy&>(*proxy_).F = std::atoi(optarg);
                break;
            case 'n':
                dynamic_cast<waffle_proxy&>(*proxy_).N = std::atoi(optarg);
                break;
            case 'd':
                dynamic_cast<waffle_proxy&>(*proxy_).D = std::atoi(optarg);
                break;
            case 's':
                dynamic_cast<waffle_proxy&>(*proxy_).object_size = std::atoi(optarg);
                break;
            default:
                usage();
                exit(-1);
        }
    }

    void *arguments[1];
//    assert(dynamic_cast<waffle_proxy&>(*proxy_).trace_location_ != "");
    std::vector<std::string> keys;
    std::vector<std::string> values;
    int N=dynamic_cast<waffle_proxy&>(*proxy_).N;
//    getKeysValues(dynamic_cast<waffle_proxy&>(*proxy_).trace_location_, keys, values);
    keys=ItemIdGenerator::generate_item_ids(N);
    std::string dummy_value(dynamic_cast<waffle_proxy&>(*proxy_).object_size,"i")
    for (auto &key: keys){
        auto value=dummy_value;
        values.push_back(value);
        key+="@"+std::to_string(1607965121);
    }
    std::cout << "Keys size before init is " << keys.size() << std::endl;
    auto id_to_client = std::make_shared<thrift_response_client_map>();
    arguments[0] = &id_to_client;
    std::cout <<"Initializing Waffle" << std::endl;
    dynamic_cast<waffle_proxy&>(*proxy_).init(keys, values, arguments);
    std::cout << "Initialized Waffle" << std::endl;
    auto proxy_server = thrift_server::create(proxy_, "waffle", id_to_client, PROXY_PORT, 1);
    std::thread proxy_serve_thread([&proxy_server] { proxy_server->serve(); });
    std::cout << "Proxy server is reachable" << std::endl;
    sleep(250);
    std::cout << "Quitting proxy server" << std::endl;
    //flush_thread(proxy_);
    //proxy_->close();
    //proxy_server->stop();
}
