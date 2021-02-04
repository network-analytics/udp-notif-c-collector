#!/bin/bash

CONTAINER_LIST=$(docker ps | grep "unyte/c-collector" | awk '{print $1}')

echo "Stopping all unyte/c-collector containers $CONTAINER_LIST"
docker stop $CONTAINER_LIST
