#!/bin/bash

# Lancer plusieurs clients en parallèle
num_clients=165  # Modifier le nombre de clients en conséquence

# Mesurer le temps d'exécution du serveur
start_time_server=$(date +%s.%N)

for ((i=0; i<$num_clients; i++)); do
  ./sender_sample 10.0.2.15 5005 ca-cert.pem
done

# Attendre que tous les clients se terminent
wait

# Mesurer le temps d'exécution du serveur
end_time_server=$(date +%s.%N)
execution_time_server=$(echo "$end_time_server - $start_time_server" | bc)

# Afficher le temps d'exécution du serveur
printf "Temps d'exécution du serveur: %.4f secondes\n" "$execution_time_server"
