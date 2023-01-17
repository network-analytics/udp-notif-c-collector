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
#include <wolfssl/ssl.h>
#include <wolfssl/options.h>
#include <signal.h>

#define MAXSEND 1000
#define SA struct sockaddr

//contextes globaux pour tous les threads
static WOLFSSL_CTX* ctx;

//gestion des shutdowns
static int cleanup;

//paramètres à transmettre dans la fonction de thread
struct params{
    int num_client;
    int socket;
    char buffer[MAXSEND];
    int size;
    WOLFSSL * ssl;
};

//pour pouvoir ctrl+c le terminal
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
    //int size = to_use->size;
    int recv_len = 0;
    int write_len = 0;
    printf("conn fd and client number = %d, %d\n", connfd, client);
    char buff[MAXSEND];
    WOLFSSL * ssl;

    //memcpy(buff, to_use->buffer, size);

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        cleanup = 1;
        return NULL;
    }

    wolfSSL_set_fd(ssl, connfd);

    if (wolfSSL_accept(ssl) != SSL_SUCCESS) {

        int e = wolfSSL_get_error(ssl, 0);

        printf("error = %d, %s\n", e, wolfSSL_ERR_reason_error_string(e));
        printf("SSL_accept failed.\n");
        return NULL;
    }
    printf("accept ok\n");

    // infinite loop for chat
    for (;;) {
        bzero(buff, MAXSEND);
   
        // read the message from client and copy it in buffer
        recv_len = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        if (recv_len > 0){
            buff[recv_len] = 0;
            printf("I heard this: \"%s\" with %d bytes\n", buff, recv_len);

        } else if (recv_len < 0){
            int readErr = wolfSSL_get_error(ssl, 0);
            if(readErr != SSL_ERROR_WANT_READ) {
                printf("SSL_read failed.\n");
                cleanup = 1;
                return NULL;
            }

        } else {
            printf("0 octets ont été lus\n");
            cleanup = 1;
            return NULL;
        }

        // and send that buffer to client
        write_len = wolfSSL_write(ssl, buff, sizeof(buff));
        if (write_len < 0){
            printf("wolfSSL_write fail.\n");
            cleanup = 1;
            return NULL;

        } else {
            // print buffer which contains the client contents
            printf("From client %d: %s \n To client : %s\n", client, buff, buff);
        }
   
        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit...\n");
            wolfSSL_shutdown(ssl);
            wolfSSL_free(ssl);
            close(connfd);

            printf("Client left return to idle state\n");
            printf("Exiting thread.\n\n");
            pthread_exit(NULL);
        }
    }
}
   
// Driver function
int main(int argc, char *argv[])
{
    if (argc != 3){
        perror("Nombre d'arguments incorrect");
        exit(1);
    }

    int sockfd;
    socklen_t cli_len;
    struct sockaddr_in servaddr, cli;
    char caCertLoc[] = "./certs/ca-cert.pem";
    char servCertLoc[] = "./certs/server-cert.pem";
    char servKeyLoc[] = "./certs/server-key.pem";
    int reuse = 1;
    socklen_t len = sizeof(reuse);
    int recv_len;

    //gestion des signaux
    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    //initialisation de la librairie
    wolfSSL_Init();
    printf("init ok\n");

    //création des contextes
    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method())) == NULL) {
        printf("wolfSSL_CTX_new error.\n");
        return 1;
    }
    printf("context ok\n");

    //vérification de la localisation des certificats
    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return 1;
    }
    printf("location ok\n");

    //utilisation des certificats du serveur
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return 1;
    }
    printf("certif ok\n");

    //gestion de la private key
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return 1;
    }
    printf("private key ok \n");
   
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket creation failed...\n");
        return 1;
    } else {
        printf("Socket successfully created..\n");
    }

    //config de l'@ serveur
    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    //permettre de réutiliser l'@ pour les autres connexions clients
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, len) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        cleanup = 1;
        return 1;
    }
   
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) < 0) {
        printf("socket bind failed...\n");
        cleanup = 1;
        return 1;
    }
    else {
        printf("Socket successfully binded..\n");
    }
    printf("serveur listening...\n");

    cli_len = sizeof(cli);
    int client_number = 1;
    char buf[MAXSEND];
   
    while(cleanup != 1){
        //création des paramètres de la fonction thread
        struct params to_send;

        //on attend en permanence qu'il y ai un client
        recv_len = (int)recvfrom(sockfd, (char *)buf, sizeof(buf), 0, (struct sockaddr*)&cli, &cli_len);

        if (cleanup == 1){
            printf("cleanup actif\n");
            return 1;
        }

        if (recv_len < 0){
            //printf("waiting for client...\n");
            continue;

        } else if (recv_len > 0) {
            memcpy(to_send.buffer, buf, sizeof(buf));
            to_send.size = recv_len;
            to_send.num_client = client_number;
            client_number++;

            if ((to_send.socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
                printf("Cannot create socket.\n");
                cleanup = 1;
            }

            if (setsockopt(to_send.socket, SOL_SOCKET, SO_REUSEADDR, &reuse, len) < 0) {
                perror("setsockopt(SO_REUSEADDR) failed");
                cleanup = 1;
                return 1;
            }

            if (setsockopt(to_send.socket, SOL_SOCKET, SO_REUSEPORT, &reuse, len) < 0) {
                perror("setsockopt(SO_REUSEADDR) failed");
                cleanup = 1;
                return 1;
            }

            if (connect(to_send.socket, (const struct sockaddr *)&cli, sizeof(cli)) != 0) {
                printf("Udp connect failed.\n");
                cleanup = 1;
                return 1;
            }


        } else {
            printf("Recvfrom failed.\n");
            cleanup = 1;
            return 1;
        }
        printf("connection established !\n");

        //gestion des threads
        if (cleanup != 1){
            pthread_t thread;
            int res = pthread_create(&thread, NULL, func_thread, (void*) &to_send);
            if(res < 0){
                perror("pb dans le thread");
                return 1;
            }
            printf("création du thread ok\n");

        } else if (cleanup == 1){
            printf("cleanup actif thread\n");
            return 1;

        } else {
            printf("bah c'est bizarre d'être ici\n");
        }

        memset((char *)&servaddr, 0, sizeof(servaddr));
    }
   
    return 0;
}
