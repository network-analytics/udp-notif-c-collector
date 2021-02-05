FROM ubuntu:focal

WORKDIR /home/unyte

COPY tmp/src src
COPY launch.sh src/launch.sh

RUN apt-get update && \
    apt-get -y install build-essential && \
    cd src && \
    make && \
    rm *.c *.h && \
    apt-get -y purge --auto-remove build-essential && \
    chmod +x launch.sh
