#!/bin/bash

# Nombre de clients à lancer en parallèle
num_clients=1  # Modifier le nombre de clients selon vos besoins

# Lancer plusieurs clients en parallèle
for ((i=0; i<$num_clients; i++)); do
  ./sender_sample 10.0.2.15 5005 ca-cert.pem
done

# Attendre que tous les clients soient terminés
wait

# Mesurer la consommation mémoire de chaque client
for pid in $(jobs -p); do
  memory_usage=$(ps -p $pid -o rss=)
  printf "Consommation mémoire du client (PID %d): %s KB\n" "$pid" "$memory_usage"
done
