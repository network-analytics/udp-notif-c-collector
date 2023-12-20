#!/bin/bash

# Lancer votre serveur en arrière-plan
./client_sample 10.0.2.15 5005 ca-cert.pem server-cert.pem server-key.pem &

# Récupérer le PID du serveur
server_pid=$!

# Initialiser le compteur de paquets côté serveur
packet_count_incoming=0

# Interface réseau côté serveur
server_interface="enp0s3"  # Remplacez par le nom de votre interface réseau

# Boucle pour mesurer le nombre de paquets entrants côté serveur toutes les 5 secondes
while true; do
    # Utiliser tcpdump pour capturer le trafic entrant sur l'interface côté serveur et compter les paquets
    packet_count_current_incoming=$(tcpdump -i $server_interface -c 10 -n -q | wc -l)

    # Calculer le nombre de paquets entrants côté serveur depuis la dernière mesure
    packets_received_server=$((packet_count_current_incoming - packet_count_incoming))

    # Afficher le nombre de paquets entrants côté serveur
    printf "Paquets entrants côté serveur depuis la dernière mesure: %d\n" "$packets_received_server"

    # Mettre à jour le compteur côté serveur pour la prochaine mesure
    packet_count_incoming=$packet_count_current_incoming

    # Attendre 5 secondes avant la prochaine mesure
    sleep 5
done

# Tuer le serveur après que la boucle est terminée
kill $server_pid
