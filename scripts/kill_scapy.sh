#!/bin/bash

SCAPY_PROCESSES=$(ps aux | grep "scapy/scripts/../src/main.py -n" | grep -v "grep" | awk '{print $2}')

# Run as root
if [ "$EUID" -ne 0 ]
then
    echo "Please run as root."
    exit
fi

for i in $SCAPY_PROCESSES ;
do 
    echo "Killing scapy $i process";
    kill -9 $i;
done
