#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <signal.h>

#define MAX 1000
#define PORT 5000
#define SA struct sockaddr

static int cleanup;
static WOLFSSL_CTX* ctx;

//TO ADD (pas de connexion ssl dans le thread alors qu'il y en a une dans le 1.3)
//ssl contexte

//TO ADD function main
//les 3 certificats pour faire fonctionner la sécurité
//struct sigaction pour utiliser les signaux
//fonction handler pour faire les signaux
//wolfSSL_Init, initialisation de la librairie
//wolfSSL_CTX_new(wolfDTLSv1_3_server_method())), création des nouveaux contextes



//TO ADD function func thread





//wolfSSL_CTX_free, free les contextes
//wolfSSL_Cleanup, cleanup des pointeurs ssl créés avant 



struct params {
    int num_client;
    int socket;
    int size;
    char message[MAX];
};

void sig_handler(const int sig)
{
    printf("\nSIGINT %d handled\n", sig);
    cleanup = 1;
    return;
}
   
// Function designed for chat between client and server.
void* func_thread(void* args)
{
    pthread_detach(pthread_self());

    struct params * to_use = (struct params *)args;
    int connfd = to_use->socket;
    int client = to_use->num_client;
    int size = to_use->size;
    char message[size];
    strcpy(message, to->message, size);

    printf("conn fd = %d\n", connfd);
    char buff[MAX];
    WOLFSSL * ssl;
    int value_read;

    //création de la connexion ssl
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        cleanup = 1;
        return NULL;
    }

    //set fd avec la socket courante
    wolfSSL_set_fd(ssl, connfd);

    //accept de la connexion
    if (wolfSSL_accept(ssl) != SSL_SUCCESS) {

        int e = wolfSSL_get_error(ssl, 0);
        printf("error = %d, %s\n", e, wolfSSL_ERR_reason_error_string(e));
        printf("SSL_accept failed.\n");
        return NULL;
    }
    if ((value_read = wolfSSL_read(ssl, buff, strlen(buff)-1)) > 0) {
        printf("heard %d bytes\n", value_read);

        buff[value_read] = 0;
        printf("I heard this: \"%s\"\n", buff);
    } else if (value_read < 0) {
        int readErr = wolfSSL_get_error(ssl, 0);
        if(readErr != SSL_ERROR_WANT_READ) {
            printf("SSL_read failed.\n");
            cleanup = 1;
            return NULL;
        }
    }

    //dernière partie à modifier
    if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
        printf("wolfSSL_write fail.\n");
        cleanup = 1;
        return NULL;
    }
    else {
        printf("Sending reply.\n");
    }

    printf("reply sent \"%s\"\n", ack);

    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(activefd);
    free(openSock);                 /* valgrind friendly free */

    printf("Client left return to idle state\n");
    printf("Exiting thread.\n\n");
    pthread_exit(openSock);


}
   
// Driver function
int main(int argc, char *argv[])
{
    if (argc != 3){
        perror("Nombre d'arguments incorrect");
        exit(1);
    }

    char caCertLoc[] = "./certs/ca-cert.pem";
    char servCertLoc[] = "./certs/server-cert.pem";
    char servKeyLoc[] = "./certs/server-key.pem";
    char buf[MAX];

    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;
    struct params to_send;

    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    //initialisation de la librairie
    wolfSSL_Init();

    //création des contextes /!\ dtls 1.2 ici
    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method())) == NULL) {
        printf("wolfSSL_CTX_new error.\n");
        return 1;
    }

    //vérifications de la localisation des contextes 
    if (wolfSSL_CTX_load_verify_locations(ctx, caCertLoc, 0) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return 1;
    }

    //utilisation des certificats
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return 1;
    }

    //utilisation de la clé privée
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return 1;
    }
   
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        cleanup = 1;
        return 1;
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    //setsockopt to reuse ip @
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        cleanup = 1;
        return 1;
    }
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    inet_aton(argv[1], &servaddr.sin_addr);
    servaddr.sin_port = htons(atoi(argv[2]));
   
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        cleanup = 1;
        return 1;
    } else {
        printf("Socket successfully binded..\n");
    }
    printf("Awaiting client connection on port %d\n", atoi(argv[2]));
   
    len = sizeof(cli);
    int client_number = 1;
   
    while(!cleanup){
        // Accept the data packet from client and verification
        printf("waiting for connection...\n");

        //premier recvfrom pour savoir si un client est connecté
        len = sizeof(cli);
        int bytesRcvd = (int)recvfrom(sockfd, (char *)buf, sizeof(buf), 0, (struct sockaddr*)&cli, &len);

        //vérification qu'il n'y ai pas une sortie à faire
        if (cleanup == 1) {
            return 1;
        }

        if (bytesRcvd < 0) {
            printf("No clients in que, enter idle state\n");
            continue;
        } else if (bytesRcvd > 0) {
            to_send.size = bytesRcvd;
            strcpy(to_send.message, buf, bytesRcvd);
            int socket_send;
            if ((socket_send = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                printf("Cannot create socket.\n");
                cleanup = 1;
            }
            to_send.socket = socket_send;
            to_send.num_client = client_number;
            client_number++;

            int res = setsockopt(socket_send, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
            if (res < 0) {
                printf("Setsockopt SO_REUSEADDR failed.\n");
                cleanup = 1;
                return 1;
            }

            res = setsockopt(socket_send, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
            if (res < 0) {
                printf("Setsockopt SO_REUSEPORT failed.\n");
                cleanup = 1;
                return 1;
            }

            if (connect(socket_send, (const struct sockaddr *)&cli, sizeof(cli)) != 0) {
                printf("Udp connect failed.\n");
                cleanup = 1;
                return 1;
            }
        } else {
            printf("Recvfrom failed.\n");
            cleanup = 1;
            return 1;
        }
        printf("one client connected\n");

        //gestion des threads
        if (cleanup != 1) {
            /* SPIN A THREAD HERE TO HANDLE "buff" and "reply/ack" */
            pthread_t thread;
            int res = pthread_create(&thread, NULL, func_thread, (void*) &to_send);
            if(res < 0){
                perror("pb dans le thread");
                return 1;
            }
        }
        else if (cleanup == 1) {
            return 1;
        } else {
            printf("I don't know what to tell ya man\n");
        }
    }
   
    // After chatting close the socket
    close(sockfd);
    return 0;
}
