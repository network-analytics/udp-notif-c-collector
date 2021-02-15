#!/bin/bash

# Run as root
if [ "$EUID" -ne 0 ]
then
  echo "Please run as root."
  exit
fi

LOG_FOLDER=$(pwd)/../logs

UNYTE_COLLECTOR=$(pwd)/..
UNYTE_SCAPY=$(pwd)/../../scapy

COLLECTOR_CLIENT=client_performance
SCAPY_SCRIPT=scripts/performance.sh

######## Scapy parameters ########
INSTANCES=60
MESSAGES=100
JSON="big"
DEST_IP="192.168.0.17"
DEST_PORT="8081"

######## C-collector parameters ########
C_MESSAGES=$(($MESSAGES * $INSTANCES))
C_TIME_BETWEEN=1000
C_VLEN=10
C_SRC=$DEST_IP
C_PORT=$DEST_PORT

######## killing all client_performance collectors and scapy processes ########
./kill_performance_client.sh
./kill_scapy.sh


######## Test ########
# TODO: make for loop to launch multiple times
sleep 1
echo "###### Start test #######" >> $LOG_FOLDER/collector.log
$UNYTE_COLLECTOR/$COLLECTOR_CLIENT $C_MESSAGES $C_TIME_BETWEEN $C_VLEN $C_SRC $C_PORT >> $LOG_FOLDER/collector.log &

sleep 1

sudo $UNYTE_SCAPY/$SCAPY_SCRIPT $INSTANCES $MESSAGES $JSON $DEST_IP $DEST_PORT -y
wait
echo "####### End test ########" >> $LOG_FOLDER/collector.log
######## End Test ########
