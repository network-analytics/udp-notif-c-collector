#!/bin/bash

if [ ! -d "tmp/src" ] 
then
    mkdir -p tmp/src
    echo "Creating tmp directory for docker context"
fi

cp ../*.c tmp/src
cp ../*.h tmp/src
cp ../Makefile tmp/src/Makefile

docker build . -t unyte/c-collector

echo "Deleting tmp folder"
rm -rf tmp
