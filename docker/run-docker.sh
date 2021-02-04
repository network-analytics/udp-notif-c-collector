#!/bin/bash

## Use docker-compose to have logs volume
docker-compose up -d

## Another command to launch the container
# docker run -d -p 8081:8081/udp unyte/c-collector:latest /home/unyte/src/client_sample
