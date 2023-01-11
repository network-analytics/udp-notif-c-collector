#include <arpa/inet.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#define MAX 1000
#define PORT 5000
#define SA struct sockaddr

// TO ADD
//ssl connexion
//ssl contexte


//TO ADD function main
//wolSSL_Init, pour l'initialisation de la librairie
//wolfSSL_CTX_new(wolfDTLSv1_3_client_method()), pour la création des contextes
//wolfSSL_CTX_load_verify_locations, pour pouvoir utiliser les certificats de sécurité
//wolfSSL_new, pour l'initialisation de la connexion 

//TO ADD thread
//wolfSSL_dtls_set_peer, à faire 
//wolfSSL_set_fd, pour cette la socket créée avec la connexion dtls
//wolfSSL_connect, pour connecter la connexion ssl 

//dans la boucle while pour la lecture et l'ecriture 
//wolfSSL_write, pour l'écriture 
//wolfSSL_read, pour la lecture
//wolfSSL_get_error, si pb
//wolfSSL_shutdown, pour shut la connexion ssl
//wolfSSL_free, pour free la connexion ssl
//wolfSSL_CTX_free, pour free les contextes
//wolfSSL_Cleanup, pour clean la connexion



void func(WOLFSSL * sockfd)
{
    char buff[MAX];
    size_t n;
    int m;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;
        n = wolfSSL_write(sockfd, buff, strlen(buff));
        if (n != strlen(buff)){
            printf("SSL_write failed");
        }

        bzero(buff, sizeof(buff));
        m = wolfSSL_read(sockfd, buff, sizeof(buff)-1); 
        if (m < 0){
            int readErr = wolfSSL_get_error(sockfd, 0);
            if (readErr != SSL_ERROR_WANT_READ) {
                printf("wolfSSL_read failed");
            }
        }
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("sortie...\n");
            break;
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
    WOLFSSL * ssl = 0;
    WOLFSSL_CTX * ctx = 0;
    char cert_array[]  = "./certs/ca-cert.pem";
    char * certs = cert_array;

    //initialisation de la librairie
    wolfSSL_Init();

    //debuggage
    wolfSSL_Debugging_ON();

    //initialisation des contextes
    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return 1;
    }

    //load certificate
    if (wolfSSL_CTX_load_verify_locations(ctx, certs, 0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", certs);
        return 1;
    }

    //assignation des contextes à la connexion 
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("unable to get ssl object");
        return 1;
    }
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else {
        printf("Socket successfully created..\n");
    }
    bzero(&servaddr, sizeof(servaddr));
    printf("coucou\n");
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    inet_aton(argv[1], &servaddr.sin_addr);
    servaddr.sin_port = htons(atoi(argv[2]));
    printf("coucou1\n");

    //set du peer du client du côté client
    wolfSSL_dtls_set_peer(ssl, &servaddr, sizeof(servaddr));
    printf("coucou2\n");

    //liaison entre la socket et la connexion ssl
    wolfSSL_set_fd(ssl, sockfd);
    printf("coucou4\n");
 
    // connect the client socket to server socket
    // if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
    //     != 0) {
    //     printf("connection with the server failed...\n");
    //     exit(0);
    // }
    // else {
    //     printf("connected to the server..\n");
    // }


    //liaison du connect dtls avec le serveur 
    printf("juste avant connexion\n");
    printf("%d\n", SSL_SUCCESS);
    printf("%d\n", wolfSSL_connect(ssl));
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        printf("bonjour\n");
	    int err1 = wolfSSL_get_error(ssl, 0);
	    printf("err = %d, %s\n", err1, wolfSSL_ERR_reason_error_string(err1));
	    printf("SSL_connect failed");
        return 1;
    }
    printf("coucou5\n");
 
    func(ssl);
    printf("coucou6\n");
 
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}
