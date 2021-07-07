# Docker container for c-collector

## Build and run 
You can use both `docker` and `podman` engines.

| Commands/scripts                     | Description             |
| ------------------------------------ |-------------------------|
| `./build-docker.sh <docker_engine>`  | Build container for current c-collector code. `<docker_engine>` can be `docker` or `podman`. Default: `docker`. |
| `./run-docker.sh <docker_engine>`    | Running container with instances to launch on the script `launch.sh`. If you need to launch more instances, you should modify `launch.sh` script and rebuild the container. `<docker_engine>` can be `docker` or `podman`. Default: `docker`. |
| `docker ps`                          | See active containers |
| `./stop-docker.sh <docker_engine>`   | Stopping all active c-collector containers. `<docker_engine>` can be `docker` or `podman`. Default: `docker`. |
| `docker logs <container_id> -f`      | See a c-collector container logs. By default, the logs are binded by a volume in the folder `logs` |
| `./clean-logs.sh`                    | Clean container volume logs in folder `logs`. |

## Notes
/!\ `client_sample.c` should listen to IP 0.0.0.0 to work on docker.
TODO: after using autotools, some of the scripts may not work anymore.