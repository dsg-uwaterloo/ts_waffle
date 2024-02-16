#!/usr/bin/env bash
#./bin/proxy_server  -l tracefiles/0.99/workloada/proxy_server_command_line_input.txt -b 1200 -r 800 -f 100 -d 100000 -c 8 -n 1 -h localhost -p 6379
./bin/proxy_server -y -a 1 -t 1 -b 120 -r 80 -f 10 -d 1000 -n 1000 -c 65 -s 1024 -h localhost -p 6379