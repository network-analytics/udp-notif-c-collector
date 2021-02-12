FROM ubuntu:focal

WORKDIR /home/unyte

COPY tmp .
COPY launch.sh launch.sh

RUN apt-get update && \
    apt-get -y install build-essential && \ 
    make && \
    apt-get -y purge --auto-remove build-essential && \
    rm **/*.c **/*.h && \
    chmod +x launch.sh
