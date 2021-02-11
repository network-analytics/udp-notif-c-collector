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
    mkdir -p tmp/src/lib
    echo "Creating tmp directory for docker context"
fi

if [ ! -d "tmp/samples" ] || [ ! -d "tmp/test" ] 
then
    mkdir -p tmp/samples/obj
    mkdir -p tmp/test
    echo "Creating tmp samples directory for docker context"
fi
if [ ! -d "tmp/obj" ] 
then
    mkdir -p tmp/obj/lib
    echo "Creating tmp object directory for docker context"
fi

cp ../src/*.c tmp/src
cp ../src/*.h tmp/src
cp ../src/lib/*.h tmp/src/lib
cp ../src/lib/*.c tmp/src/lib
cp ../samples/*.c tmp/samples
cp ../test/*.c tmp/test
cp ../obj/README.md tmp/obj/
cp ../obj/lib/README.md tmp/obj/lib
cp ../samples/obj/README.md tmp/samples/obj/
cp ../Makefile tmp/Makefile

$ENGINE build . -t unyte/c-collector

echo -e "\e[93m\e[1mMake sure you are listening on 0.0.0.0 (all ips) on client_sample.c"

echo "Deleting tmp folder"
rm -rf tmp
