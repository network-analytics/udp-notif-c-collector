#!/bin/bash

ENGINE=docker

if [ -z "$1" ] || [ "$1" == "docker" ] || [[ "$1" != "docker" && "$1" != "podman" ]]; then
    echo -e "Using default \e[1m\e[32mdocker \e[0mengine"
elif [ "$1" == "podman" ]; then
    echo -e "Using \e[1m\e[32mpodman \e[0mengine"
    ENGINE=podman
fi

if [ ! -d "tmp/src" ] 
then
    mkdir -p tmp/src
    echo "Creating tmp directory for docker context"
fi

cp ../*.c tmp/src
cp ../*.h tmp/src
cp ../Makefile tmp/src/Makefile

$ENGINE build . -t unyte/c-collector

echo "Deleting tmp folder"
rm -rf tmp
