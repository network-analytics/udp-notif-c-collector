# Docker container for c-collector

## Build and run 
- Build container : `./build-docker.sh`
- Run container : `./run-container.sh` or `docker-compose up`
- Active dockers: `docker ps`
- Stopping containers : `docker stop <container_id>` or `./stop-docker.sh` (stops all c-collector containers)
- Logs from container : `docker logs <container_id> -f`
- Cleaning logs from folder `logs`: `./clean-logs.sh`

There are 2 ways to launch the container:
1. Using script : `./run-docker.sh`
2. Using docker-compose with the docker-compose file : `docker-compose up`

