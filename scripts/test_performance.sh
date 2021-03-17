#!/bin/bash

# Run as root
# if [ "$EUID" -ne 0 ]
# then
#   echo "Please run as root."
#   exit
# fi

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
C_MESSAGES=500000
C_TIME_BETWEEN=25000
C_VLEN=50
C_SRC=$DEST_IP
C_PORT=$DEST_PORT

######## C-sender parameters ########
CS_DEST_IP=$DEST_IP
CS_PORT=$DEST_PORT
CS_MESSAGES=$C_MESSAGES
CS_GEN_ID=0
CS_RESOURCES="resources"
# CS_MSG_TYPE: 0 = GEN_ID++ | 1 = MSG_ID++
CS_MSG_TYPE=0
CS_MTU=9000
CS_MSG_SIZE=7000
CS_THREADS=1
CS_SLP_SEC=0
CS_SLP_MSG=1000

if ! ([ -L ${CS_RESOURCES} ] && [ -e ${CS_RESOURCES} ]) ; then
  echo "Creating symlink to resources"
  ln -s ../resources resources
fi

######## Test ########
for i in {1..1} ;
do 
  ######## killing all client_performance collectors and scapy processes ########
  # ./kill_performance_client.sh
  # ./kill_scapy.sh

  sleep 1
  echo "###### Start test #######" >> $LOG_FOLDER/collector.log
  $UNYTE_COLLECTOR/$COLLECTOR_CLIENT $C_MESSAGES $C_TIME_BETWEEN $C_VLEN $C_SRC $C_PORT >> $LOG_FOLDER/collector.log &
  sleep 2
  taskset -a -p 0x0000000F $!

  sleep 2

  $UNYTE_COLLECTOR/$C_SENDER $DEST_IP $CS_PORT $CS_MESSAGES $CS_GEN_ID $CS_MSG_TYPE $CS_MTU $CS_MSG_SIZE $CS_THREADS $CS_SLP_SEC $CS_SLP_MSG &
  taskset -a -p 0x000000f0 $!
  sleep 7
  $UNYTE_COLLECTOR/$C_SENDER $DEST_IP $CS_PORT $C_VLEN 49993648 $CS_MSG_TYPE $CS_MTU $CS_MSG_SIZE $CS_THREADS $CS_SLP_SEC $CS_SLP_MSG &
  taskset -a -p 0x000000f0 $!

  # sudo $UNYTE_SCAPY/$SCAPY_SCRIPT $INSTANCES $MESSAGES $JSON $C_VLEN $DEST_IP $DEST_PORT -y

  wait
  echo "####### End test ########" >> $LOG_FOLDER/collector.log
done

######## End Test ########
