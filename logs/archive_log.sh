#!/bin/bash

SIZE=$1
THREADS=$2

NOW=$(date '+%d%m%y_%H%M%S')
LOG_FILE=$LOG_FOLDER/collector_$NOW.log
FOLDER=run$SIZE-$NOW

mkdir $FOLDER

echo "Archiving in $FOLDER"

mv collector_*.log $FOLDER

cd $FOLDER

grep "Lost" collector_*.log > lost

sort -k 3 -n -t ";" lost > ordered_lost.txt
rm lost

echo "label,thread_id,packets,time" > throughput.csv
grep "Data" collector_*.log >> throughput.csv

echo -e "Message size: $SIZE \n" > stats.txt
python3 ../throughput.py $SIZE $THREADS >> stats.txt

echo -e "\n" >> stats.txt
echo "" >> stats.txt
