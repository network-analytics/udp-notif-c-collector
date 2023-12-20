#!/bin/bash

# Lancer votre client en arrière-plan
./sender_sample 10.0.2.15 5005 ca-cert.pem &

# Récupérer le PID du client
client_pid=$!

# Initialiser le compteur de paquets côté client
packet_count_client=0

# Interface réseau côté client
client_interface="enp0s3"  # Remplacez par le nom de votre interface réseau

# Boucle pour mesurer le nombre de paquets envoyés côté client toutes les 5 secondes
while true; do
    # Utiliser tcpdump pour capturer le trafic sortant côté client et compter les paquets
    packet_count_current_client=$(tcpdump -i $client_interface -c 10 -n -q | wc -l)

    # Calculer le nombre de paquets envoyés côté client depuis la dernière mesure
    packets_sent_client=$((packet_count_current_client - packet_count_client))

    # Afficher le nombre de paquets envoyés côté client
    printf "Paquets envoyés côté client depuis la dernière mesure: %d\n" "$packets_sent_client"

    # Mettre à jour le compteur côté client pour la prochaine mesure
    packet_count_client=$packet_count_current_client

    # Attendre 5 secondes avant la prochaine mesure
    sleep 5
done

# Tuer le client après que la boucle est terminée
kill $client_pid

#!/bin/bash

# Lancer votre client en arrière-plan
./sender_sample 10.0.2.15 5005 ca-cert.pem &

# Récupérer le PID du client
client_pid=$!

# Initialiser le compteur de paquets côté client
packet_count_outgoing=0

# Interface réseau côté client
client_interface="enp0s3"  # Remplacez par le nom de votre interface réseau
server_ip="10.0.2.15"  # Remplacez par l'adresse IP du serveur

# Boucle pour mesurer le nombre de paquets envoyés côté client toutes les 5 secondes
while true; do
    # Utiliser tcpdump pour capturer le trafic sortant côté client et compter les paquets
    packet_count_current_outgoing=$(tcpdump -i $client_interface -c 10 -n -q "dst host $server_ip" | wc -l)

    # Calculer le nombre de paquets envoyés côté client depuis la dernière mesure
    packets_sent_client=$((packet_count_current_outgoing - packet_count_outgoing))

    # Afficher le nombre de paquets envoyés côté client
    printf "Paquets envoyés côté client depuis la dernière mesure: %d\n" "$packets_sent_client"

    # Mettre à jour le compteur côté client pour la prochaine mesure
    packet_count_outgoing=$packet_count_current_outgoing

    # Attendre 5 secondes avant la prochaine mesure
    sleep 5
done

# Tuer le client après que la boucle est terminée
kill $client_pid
