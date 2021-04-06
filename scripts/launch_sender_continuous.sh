#!/bin/bash

############## SENDER ARGUMENTS ##############
# <address> <port> <gen_id> <mtu> <msg_bytes_size> <milisec_sleep> <messages_mod> <jitter>
# CS_IP="192.168.0.17"
CS_IP="10.212.252.11"
CS_PORT=8081
CS_MTU=9000
CS_MSG_BYTES=8950
CS_SLEEP_MS=70
CS_SLEEP_MSG=300
CS_JITTER=0.3
CS_INTERFACE=""

if [[ $# -eq 0 ]]; then
  echo "No IP supplied. Using default $CS_IP:$CS_PORT";
elif [[ $# -eq 1 ]]; then
  CS_IP=$1
  echo "Using IP supplied: $CS_IP:$CS_PORT";
else
  CS_IP=$1
  CS_PORT=$2
  echo "Using IP:port supplied: $CS_IP:$CS_PORT";
fi

SENDER=$(pwd)/../sender_continuous
CS_RESOURCES="resources"

if ! ([ -L ${CS_RESOURCES} ] && [ -e ${CS_RESOURCES} ]) ; then
  echo "Creating symlink to resources"
  ln -s ../resources $CS_RESOURCES
fi

$SENDER $CS_IP $CS_PORT 0 $CS_MTU $CS_MSG_BYTES $CS_SLEEP_MS $CS_SLEEP_MSG $CS_JITTER $CS_INTERFACE &
$SENDER $CS_IP $CS_PORT 232 $CS_MTU $CS_MSG_BYTES $CS_SLEEP_MS $CS_SLEEP_MSG $CS_JITTER $CS_INTERFACE &
$SENDER $CS_IP $CS_PORT 453 $CS_MTU $CS_MSG_BYTES $CS_SLEEP_MS $CS_SLEEP_MSG $CS_JITTER $CS_INTERFACE &
$SENDER $CS_IP $CS_PORT 235 $CS_MTU $CS_MSG_BYTES $CS_SLEEP_MS $CS_SLEEP_MSG $CS_JITTER $CS_INTERFACE &
