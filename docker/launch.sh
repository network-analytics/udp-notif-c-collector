#!/bin/bash

SRC=/home/unyte

$SRC/client_sample > /home/unyte/logs/client1.log &
$SRC/client_sample > /home/unyte/logs/client2.log &
$SRC/client_sample > /home/unyte/logs/client3.log &
$SRC/client_sample > /home/unyte/logs/client4.log
