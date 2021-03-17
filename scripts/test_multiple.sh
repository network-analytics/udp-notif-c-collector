#!/bin/bash

#sudo ./test_multiple.sh 192.168.1.10 8081 100000 10000

############## CLIENT ARGUMENTS ##############
#MESSAGES=10000
C_INTERVAL=50000
#VLEN=100
# IP="192.168.1.10"
IP="192.168.0.17"
PORT="8081"
#array of vlen sizes for recv_mmsg
C_ARR_VLEN=( 50 )
C_ARR_MESSAGES=( 500000 )
# TODO: tests differents params
C_PARSERS=10
#20MB
C_SOCKET_BUFF=20971520
C_OUTPUT_Q_SIZE=1000
C_PARSER_Q_SIZE=500

############## SENDER ARGUMENTS ##############
#array of maximum transmission units
#ARR_MTU=( 1500 )
#array of input queue buffer sizes
#ARR_INPUT=( 5000 )
#array of output queue buffer sizes
#ARR_OUTPUT=( 5000 )
GEN_ID=0
END_VAL=49993648
CS_MSG_TYPE=0
MTU=9000
MSG_SIZE=8950


############## ENVIRONMENT ##############
CLIENT=$(pwd)/../client_performance
SENDER=$(pwd)/../sender_performance
LOG_FOLDER=$(pwd)/../logs
CS_RESOURCES="resources"

if ! ([ -L ${CS_RESOURCES} ] && [ -e ${CS_RESOURCES} ]) ; then
  echo "Creating symlink to resources"
  ln -s ../resources $CS_RESOURCES
fi

############## TESTING ##############
for MESSAGES in "${C_ARR_MESSAGES[@]}"
do
    for VLEN in "${C_ARR_VLEN[@]}"
    do
        $CLIENT $MESSAGES $C_INTERVAL $VLEN $IP $PORT $C_PARSERS $C_SOCKET_BUFF $C_OUTPUT_Q_SIZE $C_PARSER_Q_SIZE >> $LOG_FOLDER/collector.log &
        sleep 2
        taskset -a -p 0x0000000F $!
        sleep 2
        $SENDER $IP $PORT $MESSAGES $GEN_ID $CS_MSG_TYPE $MSG_SIZE $MSG_SIZE &
        taskset -a -p 0x000000f0 $!
        sleep 8
        $SENDER $IP $PORT $VLEN $END_VAL $CS_MSG_TYPE $MSG_SIZE $MSG_SIZE &
        taskset -a -p 0x000000f0 $!
        wait
    done
done