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
INSTANCES=10
MESSAGES=10
JSON="big"
DEST_IP="192.168.17.0"
DEST_PORT="8081"

######## killing all client_performance collectors ########
./kill_performance_client.sh

$UNYTE_COLLECTOR/$COLLECTOR_CLIENT >> $LOG_FOLDER/collector.log &

sleep 1

sudo $UNYTE_SCAPY/$SCAPY_SCRIPT $INSTANCES $MESSAGES $JSON $DEST_IP $DEST_PORT -y
