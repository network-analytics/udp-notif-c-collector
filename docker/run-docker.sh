#!/bin/bash

ENGINE=docker

if [ -z "$1" ] || [ "$1" == "docker" ] || [[ "$1" != "docker" && "$1" != "podman" ]]; then
    echo -e "Using default \e[1m\e[32mdocker \e[0mengine"
elif [ "$1" == "podman" ]; then
    echo -e "Using \e[1m\e[32mpodman \e[0mengine"
    ENGINE=podman
fi

$ENGINE run -d -p 8081:8081/udp -v "$(pwd)"/logs:/home/unyte/logs unyte/c-collector:latest /home/unyte/src/launch.sh
