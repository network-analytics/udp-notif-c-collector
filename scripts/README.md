# Scrips for c-collector
Scripts used to benchmark the collector

## Setup used
- Buffer UDP 25MB: `sudo sysctl -w net.core.rmem_max=26214400`
- Buffer socket set in code: 25MB

## Monitoring packet loss
- `watch -n0.2 cat /proc/net/udp`
