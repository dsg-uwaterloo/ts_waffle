#ifndef TSCONFIG_H
#define TSCONFIG_H

#include <string>

namespace benchmarkConfig
{
    extern int object_size;
    extern int no_items;
    extern int client_batch_size;
    extern int num_clients;
    extern std::string trace_location;
}

namespace waffleConfig
{
    extern int num_cores;
    extern int cacheBatches;
    extern int security_batch_size;
    extern int p_threads;
    extern std::string output_location;
    extern std::string trace_location;
    extern bool latency;
    extern int B;
    extern int R;
    extern int F;
    extern int N;
    extern int D;
    extern int object_size;
}

#endif // TSCONFIG_H