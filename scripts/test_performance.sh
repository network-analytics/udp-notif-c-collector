#!/bin/bash

# Run as root
if [ "$EUID" -ne 0 ]
then
  echo "Please run as root."
  exit
fi

LOG_FOLDER=$(pwd)/../logs

# unyte collector projects paths
UNYTE_COLLECTOR=$(pwd)/..
UNYTE_SCAPY=$(pwd)/../../scapy

COLLECTOR_CLIENT=client_performance
SCAPY_SCRIPT=scripts/performance.sh
C_SENDER=sender_performance

######## Scapy parameters ########
INSTANCES=200
MESSAGES=100
JSON="big"
DEST_IP="192.168.0.17"
DEST_PORT="8081"

######## C-collector parameters ########
# C_MESSAGES=$(($MESSAGES * $INSTANCES))
C_MESSAGES=100000
C_TIME_BETWEEN=10000
C_VLEN=50
C_SRC=$DEST_IP
C_PORT=$DEST_PORT

######## C-sender parameters ########
CS_DEST_IP=$DEST_IP
CS_PORT=$DEST_PORT
CS_MESSAGES=$C_MESSAGES
CS_GEN_ID=0

######## Test ########

for i in {1..1} ;
do 
  ######## killing all client_performance collectors and scapy processes ########
  ./kill_performance_client.sh
  ./kill_scapy.sh

  sleep 1
  echo "###### Start test #######" >> $LOG_FOLDER/collector.log
  taskset -ca 0 $($UNYTE_COLLECTOR/$COLLECTOR_CLIENT $C_MESSAGES $C_TIME_BETWEEN $C_VLEN $C_SRC $C_PORT >> $LOG_FOLDER/collector.log) &

  sleep 2

  taskset -ca 1 $UNYTE_COLLECTOR/$C_SENDER $DEST_IP $CS_PORT $CS_MESSAGES $CS_GEN_ID
  taskset --cpu-list 1-7 $UNYTE_COLLECTOR/$C_SENDER $DEST_IP $CS_PORT $C_VLEN 49993648

  # sudo $UNYTE_SCAPY/$SCAPY_SCRIPT $INSTANCES $MESSAGES $JSON $C_VLEN $DEST_IP $DEST_PORT -y

  wait
  echo "####### End test ########" >> $LOG_FOLDER/collector.log
done

######## End Test ########
