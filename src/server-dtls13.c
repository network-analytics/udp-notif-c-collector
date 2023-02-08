#include <wolfssl/options.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <wolfssl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "dtls-common.h"

WOLFSSL_CTX*  ctx = NULL;
WOLFSSL*      ssl = NULL;
int           listenfd = INVALID_SOCKET;   /* Initialize our socket */

static void sig_handler(const int sig);
static void free_resources(void);

struct params {
    WOLFSSL * ssl;
};


int cleanup_setup(){
    free_resources();
    wolfSSL_Cleanup();
    return 1;
}


void * func_thread(void * args){
    pthread_detach(pthread_self());
    struct params * to_use = (struct params *)args;
    int recvLen = 0;    /* length of message */
    char ack[] = "I hear you fashizzle!\n";
    char buff[MAXLINE];

    signal(SIGINT, sig_handler);

    while (1) {
        if ((recvLen = wolfSSL_read(to_use->ssl, buff, sizeof(buff)-1)) > 0) {
            printf("heard %d bytes\n", recvLen);

            buff[recvLen] = '\0';
            printf("I heard this: %s\n", buff);
        }
        else if (recvLen <= 0) {
            int err = wolfSSL_get_error(ssl, 0);
            if (err == WOLFSSL_ERROR_ZERO_RETURN) /* Received shutdown */
                break;
            fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "SSL_read failed.\n");
            cleanup_setup();
        }
        printf("Sending reply.\n");
        if (wolfSSL_write(to_use->ssl, ack, sizeof(ack)) < 0) {
            int err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "wolfSSL_write failed.\n");
            cleanup_setup();
        }
    }
    printf("reply sent \"%s\"\n", ack);

    pthread_exit(NULL);
}



int main()
{
    /* Loc short for "location" */
    int           exitVal = 1;
    struct sockaddr_in servAddr;        /* our server's address */
    struct sockaddr_in cliaddr;         /* the client's address */
    int           ret;
    int           err;
    socklen_t     cliLen;
    char          buff[MAXLINE];   /* the incoming message */

    /* Initialize wolfSSL before assigning ctx */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init error.\n");
        return exitVal;
    }

    /* No-op when debugging is not compiled in */
    //wolfSSL_Debugging_ON();

    /* Set ctx to DTLS 1.3 */
    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_3_server_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        cleanup_setup();
    }
    /* Load CA certificates */
    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", caCertLoc);
        cleanup_setup();
    }
    /* Load server certificates */
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servCertLoc);
        cleanup_setup();
    }
    /* Load server Keys */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servKeyLoc);
        cleanup_setup();
    }

    /* Create a UDP/IP socket */
    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        cleanup_setup();
    }
    printf("Socket allocated\n");

    int reuse = 1;
    socklen_t len = sizeof(reuse);
    int res = setsockopt(args->activefd, SOL_SOCKET, SO_REUSEADDR, &reuse, len);
    if (res < 0) {
        printf("Setsockopt SO_REUSEADDR failed.\n");
        cleanup = 1;
        return 1;
    }

    memset((char *)&servAddr, 0, sizeof(servAddr));
    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    /* Bind Socket */
    if (bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind()");
        cleanup_setup();
    }

    while (1) {
        printf("Awaiting client connection on port %d\n", SERV_PORT);

        cliLen = sizeof(cliaddr);
        ret = (int)recvfrom(listenfd, (char *)&buff, sizeof(buff), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &cliLen);

        if (ret < 0) {
            perror("recvfrom()");
            cleanup_setup();
        }
        else if (ret == 0) {
            fprintf(stderr, "recvfrom zero return\n");
            cleanup_setup();
        }

        /* Create the WOLFSSL Object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "wolfSSL_new error.\n");
            cleanup_setup();
        }

        if (wolfSSL_dtls_set_peer(ssl, &cliaddr, cliLen) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_dtls_set_peer error.\n");
            cleanup_setup();
        }

        if (wolfSSL_set_fd(ssl, listenfd) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_set_fd error.\n");
            break;
        }

        if (wolfSSL_accept(ssl) != SSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "SSL_accept failed.\n");
            cleanup_setup();
        }
        //showConnInfo(ssl);
        struct params to_send;
        to_send.ssl = ssl;
        pthread_t thread;
        int res = pthread_create(&thread, NULL, func_thread, (void*) &to_send);
        if(res < 0){
            perror("pb dans le thread");
            return 1;
        }
        printf("création du thread ok\n");

        pthread_join(thread, NULL);

        /* Attempt a full shutdown */
        ret = wolfSSL_shutdown(ssl);
        if (ret == WOLFSSL_SHUTDOWN_NOT_DONE)
            ret = wolfSSL_shutdown(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "wolfSSL_shutdown failed\n");
        }
        wolfSSL_free(ssl);
        ssl = NULL;

        printf("Awaiting new connection\n");
    }
    
    exitVal = 0;
    return exitVal;
}


static void sig_handler(const int sig)
{
    (void)sig;
    free_resources();
    wolfSSL_Cleanup();
}

static void free_resources(void)
{
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        ssl = NULL;
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (listenfd != INVALID_SOCKET) {
        close(listenfd);
        listenfd = INVALID_SOCKET;
    }
}
