#!/bin/bash

# Lancer votre serveur en arrière-plan
./client_sample 10.0.2.15 5005 ca-cert.pem server-cert.pem server-key.pem &

# Récupérer le PID du serveur
server_pid=$!

# Mesurer la consommation mémoire toutes les 5 secondes
while true; do
    # Utiliser ps pour obtenir la consommation mémoire (RSS)
    memory_usage=$(ps -p $server_pid -o rss=)
    
    # Afficher la consommation mémoire
    printf "Consommation mémoire du serveur (PID %d): %s KB\n" "$server_pid" "$memory_usage"

    # Attendre 5 secondes avant la prochaine mesure
    sleep 5
done
