#!/bin/bash

PORTS="3450
4000
5320
5321
5322
5323
6520
6521
6522
6523
6524"

FILES="client1.log
client2.log
client3.log
client4.log"

for port in $PORTS
do
    echo "********* Port $port **********"
    for file in $FILES
    do
        count=$( cat $file | grep $port | wc -l )
        echo "count $file : $count"
    done
done
