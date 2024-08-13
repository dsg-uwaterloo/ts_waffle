#include "tsconfig.h"
#include <string>

namespace benchmarkConfig
{
    int object_size = 1024;
    int no_items = 1000;
    int client_batch_size = 50;
    int num_clients = 1;
    std::string trace_location = "tracefiles/TS_data_binary.bin";
}

namespace waffleConfig
{
    int num_cores = 1;
    int cacheBatches = 2;
    int security_batch_size = 3;
    int p_threads = 32;
    std::string output_location = "log";
    std::string trace_location = "";
    bool latency = true;
    int B = 1200;
    int R = 800;
    int F = 100;
    int N = 100000;
    int D = 100000;
    int object_size = 1024;
}