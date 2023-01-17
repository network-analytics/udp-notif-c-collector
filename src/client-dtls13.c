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


#define MAXSEND 1000
#define SA struct sockaddr

void func(WOLFSSL * ssl)
{
    char buff[MAXSEND];
    size_t write;
    int read;
    int n;

    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;

        write = wolfSSL_write(ssl, buff, sizeof(buff));
        if (write != strlen(buff)){
            printf("SSL_write failed");
        }
        printf("To Server : %s", buff);
        bzero(buff, sizeof(buff));

        read = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        if (read < 0) {
            int readErr = wolfSSL_get_error(ssl, 0);
            if (readErr != SSL_ERROR_WANT_READ) {
                printf("wolfSSL_read failed");
            }
        }
        buff[read] = '\0';
        printf("From Server : %s", buff);
        
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("sortie...\n");
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
    int sockfd = 0;
    struct sockaddr_in servaddr;
    WOLFSSL * ssl = 0;
    WOLFSSL_CTX * ctx = 0;
    char cert_array[]  = "./certs/ca-cert.pem";
    char * certs = cert_array;

    //initialisation de la librairie wolfssl
    wolfSSL_Init();
    printf("init ok\n");

    //création des contextes
    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return 1;
    }
    printf("context ok\n");

    //vérification de la localisation des certificats
    if (wolfSSL_CTX_load_verify_locations(ctx, certs, 0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", certs);
        return 1;
    }
    printf("location ok\n");

    //création de la connexion ssl
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("unable to get ssl object");
        return 1;
    }
    printf("new ok\n");

    //config de l'@ du serveur
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    //liaison du client avec le serveur
    wolfSSL_dtls_set_peer(ssl, &servaddr, sizeof(servaddr));
    printf("set peer ok\n");
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket creation failed...\n");
        return 1;
    } else {
        printf("Socket successfully created..\n");
    }

    //liaison de la socket créée et de la connexion ssl
    wolfSSL_set_fd(ssl, sockfd);
    printf("set fd ok\n");

    //connexion ssl, fonctionne pas 
    printf("avant connect\n");
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        printf("coucou\n");
	    int err1 = wolfSSL_get_error(ssl, 0);
	    printf("err = %d, %s\n", err1, wolfSSL_ERR_reason_error_string(err1));
	    printf("SSL_connect failed");
        return 1;
    }

    printf("connect ok\n");
 
    func(ssl);
 
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}
