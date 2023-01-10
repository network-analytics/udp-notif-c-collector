#include <arpa/inet.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX 1000
#define PORT 5000
#define SA struct sockaddr

// TO ADD
//ssl connexion
//ssl contexte


//TO ADD function 
//wolSSL_Init, pour l'initialisation de la librairie
//wolfDTLSv1_3_client_method(), pour la création des contextes
//wolfSSL_CTX_load_verify_locations, pour pouvoir utiliser les certificats de sécurité
//wolfSSL_new, pour l'initialisation de la connexion 

//wolfSSL_dtls_set_peer ??
//wolfSSL_set_fd, pour cette la socket créée avec la connexion dtls
//wolfSSL_connect, pour connecter la connexion ssl 

//dans la boucle while pour la lecture et l'ecriture 
//wolfSSL_write, pour l'écriture 
//wolfSSL_read, pour la lecture
//wolfSSL_get_error, si pb
//wolfSSL_shutdown, pour shut la connexion ssl
//wolfSSL_CTX_free, pour free les contextes
//wolfSSL_Cleanup, pour clean la connexion



void func(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;
        write(sockfd, buff, sizeof(buff)); //mettre la fonction "wolfSSL_write"
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff)); //mettre la fonction "wolfSSL_read"
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("sortie...\n");
            close(sockfd);
            return;
        }
    }
}
 
int main(int argc, char *argv[])
{
    if (argc != 3){
        perror("Nombre d'arguments incorrect");
        exit(1);
    }
    int sockfd;
    struct sockaddr_in servaddr;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    inet_aton(argv[1], &servaddr.sin_addr);
    servaddr.sin_port = htons(atoi(argv[2]));
 
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
 
    func(sockfd);
 
    close(sockfd);
}