#!/bin/bash

############## CLIENT ARGUMENTS ##############
# <max_to_receive> <time_between_log> <vlen> <src_IP> <src_port> <nb_parsers> <socket_buff_size> <output_q_size> <parser_q_size>
C_MESSAGES=500000
C_TIME_BETWEEN=50000
C_VLEN=50
C_IP="192.168.0.17"
C_PORT="8081"
C_PARSERS=10
C_SOCKET_BUFF=20971520
C_OUTPUT_Q_SIZE=1000
C_PARSER_Q_SIZE=500

############## SENDER ARGUMENTS ##############
# <address> <port> <message_to_send> <gen_id> <type_msg> <mtu> <msg_bytes_size> <threads> <milisec_sleep> <messages_mod>
CS_IP=$C_IP
CS_PORT=$C_PORT
CS_MESSAGES=$C_MESSAGES
CS_GEN_ID=0
CS_END_VAL=49993648
CS_TYPE=0
CS_MTU=9000
CS_MSG_BYTES=8950
CS_THREADS=1
CS_SLEEP_MS=0
CS_SLEEP_MSG=9000

############## ENVIRONMENT ##############
NOW=$(date '+%d%m%y_%H%M%S')
CLIENT=$(pwd)/../client_performance
SENDER=$(pwd)/../sender_performance
LOG_FOLDER=$(pwd)/../logs
CS_RESOURCES="resources"
LOG_FILE=$LOG_FOLDER/collector_$NOW.log
# LOG_FILE=$LOG_FOLDER/collector.log

if ! ([ -L ${CS_RESOURCES} ] && [ -e ${CS_RESOURCES} ]) ; then
  echo "Creating symlink to resources"
  ln -s ../resources $CS_RESOURCES
fi

LOSS="0.000000"

SLEEP_MS=( 300 150 170 60 50 40 30 20 10 0 )
SLEEP_MSGS=( 3000 7000 )
THREADS=( 2 3 4 )

# SLEEP_MS=( 100 )
# SLEEP_MSGS=( 7000 8000 9000 10000 11000 )
# THREADS=( 1 )

############## TESTING ##############
for THREAD in "${THREADS[@]}"
do
  for SL_MSG in "${SLEEP_MSGS[@]}"
  do
    for SL_MS in "${SLEEP_MS[@]}"
    do
      echo "************* $THREAD|$SL_MSG|$SL_MS *************" >> $LOG_FILE
      $CLIENT $C_MESSAGES $C_TIME_BETWEEN $C_VLEN $C_IP $C_PORT $C_PARSERS $C_SOCKET_BUFF $C_OUTPUT_Q_SIZE $C_PARSER_Q_SIZE >> $LOG_FILE &
      sleep 2
      taskset -a -p 0x0000000F $!
      sleep 2
      $SENDER $CS_IP $CS_PORT $CS_MESSAGES $CS_GEN_ID $CS_TYPE $CS_MTU $CS_MSG_BYTES $THREAD $SL_MS $SL_MSG &
      taskset -a -p 0x00000030 $!
      wait $!

      $SENDER $CS_IP $CS_PORT $C_VLEN $CS_END_VAL $CS_TYPE $CS_MTU $CS_MSG_BYTES $THREAD $SL_MS $SL_MSG &
      taskset -a -p 0x00000030 $!
      wait

      FILE_LAST_LINE=$(tail -n 1 $LOG_FILE)
      LOSS=${FILE_LAST_LINE##*;}
      echo "********* END $THREAD|$SL_MSG|$SL_MS ************" >> $LOG_FILE;
      if [[ $LOSS != "0.000000" ]]; then
        echo "Loss here: $LOSS %";
        break 3;
      fi
    done
  done
done