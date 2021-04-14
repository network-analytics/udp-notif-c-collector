#!/bin/bash

############## CLIENT ARGUMENTS ##############
# <address> <port> <vlen> <nb_parsers> <output_queue> <parser_queue> <socket_buff> <number_gids> <messages_to_recv>
C_IP="192.168.1.89"
C_PORT=10001
C_VLEN=50
C_PARSERS=10
C_OUTPUT_Q_SIZE=1000
C_PARSER_Q_SIZE=500
# C_SOCKET_BUFF=26214400 # 25 MB
C_SOCKET_BUFF=104857600 # 100MB
C_NB_GIDS=5
C_MESSAGES=50000
C_TIME_BETWEEN=10000
C_THREADS=9

ITERATIONS=5

if [[ $# -eq 0 ]]; then
  echo "No IP supplied. Using default $C_IP:$C_PORT";
elif [[ $# -eq 1 ]]; then
  C_IP=$1
  echo "Using IP supplied: $C_IP:$C_PORT";
elif [[ $# -eq 1 ]]; then
  C_IP=$1
  C_PORT=$2
  echo "Using IP:port supplied: $C_IP:$C_PORT";
else
  C_IP=$1
  C_PORT=$2
  echo "Using IP:port supplied: $C_IP:$C_PORT";
  ITERATIONS=$3
fi

CLIENT=$(pwd)/../client_continuous
LOG_FOLDER=$(pwd)/../logs

for (( c=1; c<=$ITERATIONS; c++ ))
do  
  echo "Launching run $c"

  NOW=$(date '+%d%m%y_%H%M%S')
  LOG_FILE=$LOG_FOLDER/collector_$NOW.log
  # LOG_FILE=$LOG_FOLDER/collector.log

  $CLIENT $C_IP $C_PORT $C_VLEN $C_PARSERS $C_OUTPUT_Q_SIZE $C_PARSER_Q_SIZE $C_SOCKET_BUFF $C_NB_GIDS $C_MESSAGES $C_TIME_BETWEEN $C_THREADS >> $LOG_FILE 
done
