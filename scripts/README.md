# Scrips for c-collector
Scripts used to benchmark the collector

## Setup used
- Buffer UDP 25MB: `sudo sysctl -w net.core.rmem_max=26214400`
- Buffer socket set in code: 25MB

## Monitoring packet loss
- `watch -n0.2 cat /proc/net/udp`


## Tests on the lab
- `./launch_sender_continuous.sh <ip?> <port?>`
- `./launch_client_continuous.sh <ip?> <port?>`