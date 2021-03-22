#!/bin/bash

############## CLIENT ARGUMENTS ##############
# <address> <port> <vlen> <nb_parsers> <output_queue> <parser_queue> <socket_buff> <number_gids> <messages_to_recv>
C_IP="192.168.0.17"
C_PORT="8081"
C_VLEN=50
C_PARSERS=10
C_OUTPUT_Q_SIZE=1000
C_PARSER_Q_SIZE=500
C_SOCKET_BUFF=26214400
C_NB_GIDS=5
C_MESSAGES=50000
C_TIME_BETWEEN=10000

NOW=$(date '+%d%m%y_%H%M%S')
CLIENT=$(pwd)/../client_continuous
LOG_FOLDER=$(pwd)/../logs
LOG_FILE=$LOG_FOLDER/collector_$NOW.log
# LOG_FILE=$LOG_FOLDER/collector.log

$CLIENT $C_IP $C_PORT $C_VLEN $C_PARSERS $C_OUTPUT_Q_SIZE $C_PARSER_Q_SIZE $C_SOCKET_BUFF $C_NB_GIDS $C_MESSAGES $C_TIME_BETWEEN >> $LOG_FILE 
