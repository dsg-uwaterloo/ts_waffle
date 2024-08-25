# Waffle

**NOTE**: Although Waffle is intended & recommended to run on 3 different machines, we can still run all the three processes on the same machine.

Waffle is the first system to hide data access patterns adaptively, without requiring to
known the input data access distribution, under a passive persistent
adversary. Waffle incurs a constant bandwidth and client-side storage
overhead, both of which can be configured by an application
owner.

## Requirements

* cmake-3.5+
* redis-server (https://redis.io/docs/getting-started/installation/install-redis-on-linux/ )

## Building

After installing the requirements, run

```
sh build.sh
```
inside waffle folder

Troubleshoot:
* in cmakebuild/external/thrift_ep, libthrift.a and libthriftnb.a are be named libthriftd.a and libthriftnbd. very weird. (I manually rename them during build…)
* tacopie should not exist in usr/local/lib. If it is, cmake will not install tacopie in external/cpp_redis/lib

## Running 

Running Waffle requires at least 3 machines:

1. Client with a CLIENT_IP and CLIENT_PORT
2. Proxy with a PROXY_IP
3. Redis backing storage server with STORAGE_SERVER_IP and STORAGE_PORT

First start the storage server

```
sudo service redis-server start
```

Then start the proxy:

```
./bin/proxy_server -h <STORAGE_SERVER_IP> -p <STORAGE_PORT>

(Example: ./bin/proxy_server -h localhost -p 6379 )
```

Waffle will now initialize. After the proxy says it's reachable launch the benchmark code:

```
./bin/proxy_benchmark -h <PROXY_IP> -p <PROXY_PORT>

(Example: ./bin/proxy_benchmark -h localhost -p 9090 )
```

After completion the benchmark will display the throughput during the run. There will be a new folder in the data folder that contains one file for each client displaying the latency of each operation in nanoseconds.

Note that although it is expected that each of these processes run on different machine, they can all be run on a single machine on different ports using the localhost IP.

## What's new in TS-Waffle

* The trace file is generated from `tswaffle/waffle/utils/generate_TS_tracefile.cpp`. Each put query puts a serialized binary bucket into storage, each get query gets the sum of the range specified in the query.
* Each put query will overwrite the oldest bucket for that key in `waffle_proxy::remove_oldest_data`. Each get query will be split into multiple database get queries in `waffle_proxy::consumer_thread`.
* Global params are in `tswaffle/waffle/proxy/src/waffle_proxy.cpp`
