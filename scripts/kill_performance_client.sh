#!/bin/bash

# Run as root
if [ "$EUID" -ne 0 ]
then
  echo "Please run as root."
  exit
fi

PERF_CLIENT_PROCESSES=$(ps aux | grep "client_performance" | grep -v "grep" | awk '{print $2}')

for i in $PERF_CLIENT_PROCESSES ;
do 
    echo "Killing $i client_performance process";
    kill -9 $i;
done
