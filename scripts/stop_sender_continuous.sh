#!/bin/bash

SENDER_PROCESSES=$(ps aux | grep "sender_continuous" | grep -v "grep" | awk '{print $2}')

for i in $SENDER_PROCESSES ;
do 
    echo "Killing sender_continuous $i process";
    kill -9 $i;
done
