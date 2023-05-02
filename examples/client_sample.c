// #include <stdio.h>
// #include <unistd.h>
// #include <arpa/inet.h>

// #include "../src/hexdump.h"
// #include "../src/unyte_udp_collector.h"

// #define USED_VLEN 1
// #define MAX_TO_RECEIVE 10

// int main(int argc, char *argv[])
// {
//   if (argc != 3)
//   {
//     printf("Error: arguments not valid\n");
//     printf("Usage: ./client_sample <ip> <port>\n");
//     exit(1);
//   }

//   // Create a udp socket with default socket buffer
//   int socketfd = unyte_udp_create_socket(argv[1], argv[2], DEFAULT_SK_BUFF_SIZE);
//   printf("Listening on %s:%s\n", argv[1], argv[2]);

//   // Initialize collector options
//   unyte_udp_options_t options = {0};
//   options.recvmmsg_vlen = USED_VLEN;
//   options.socket_fd = socketfd; // passing socket file descriptor to listen to
//   options.msg_dst_ip = false;   // destination IP not parsed from IP packet to improve performance
//   options.nb_parsers = 1;

//   /* Initialize collector */
//   unyte_udp_collector_t *collector = unyte_udp_start_collector(&options);
//   int recv_count = 0;
//   int max = MAX_TO_RECEIVE;

//   while (recv_count < max)
//   {
//     /* Read queue */
//     void *seg_pointer = unyte_udp_queue_read(collector->queue);
//     if (seg_pointer == NULL)
//     {
//       printf("seg_pointer null\n");
//       fflush(stdout);
//     }
//     unyte_seg_met_t *seg = (unyte_seg_met_t *)seg_pointer;

//     // printf("unyte_udp_get_version: %u\n", unyte_udp_get_version(seg));
//     // printf("unyte_udp_get_space: %u\n", unyte_udp_get_space(seg));
//     // printf("unyte_udp_get_media_type: %u\n", unyte_udp_get_media_type(seg));
//     printf("ok client sample\n");
//     printf("unyte_udp_get_header_length: %u\n", unyte_udp_get_header_length(seg));
//     printf("unyte_udp_get_message_length: %u\n", unyte_udp_get_message_length(seg));
//     // printf("unyte_udp_get_observation_domain_id: %u\n", unyte_udp_get_observation_domain_id(seg));
//     // printf("unyte_udp_get_message_id: %u\n", unyte_udp_get_message_id(seg));
//     // printf("unyte_udp_get_src[family]: %u\n", unyte_udp_get_src(seg)->ss_family);
//     // printf("unyte_udp_get_dest_addr[family]: %u\n", unyte_udp_get_dest_addr(seg) == NULL ? 0 : unyte_udp_get_dest_addr(seg)->ss_family); // NULL if options.msg_dst_ip is set to false (default)
//     // char ip_canonical[100];
//     // if (unyte_udp_get_src(seg)->ss_family == AF_INET) {
//     //   printf("src IPv4: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_addr.s_addr, ip_canonical, sizeof ip_canonical));
//     //   printf("src port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_port));
//     // } else {
//     //   printf("src IPv6: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_addr.s6_addr, ip_canonical, sizeof ip_canonical));
//     //   printf("src port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_port));
//     // }
//     // Only if options.msg_dst_ip is set to true
//     // printf("ok client sample\n");
//     if (unyte_udp_get_dest_addr(seg) != NULL) {
//       char ip_dest_canonical[100];
//       if (unyte_udp_get_dest_addr(seg)->ss_family == AF_INET) {
//         printf("dest IPv4: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_addr.s_addr, ip_dest_canonical, sizeof ip_dest_canonical));
//         printf("dest port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_port));
//       } else {
//         printf("dest IPv6: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_addr.s6_addr, ip_dest_canonical, sizeof ip_dest_canonical));
//         printf("dest port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_port));
//       }
//     }
//     // printf("unyte_udp_get_payload: %.*s\n", unyte_udp_get_payload_length(seg), unyte_udp_get_payload(seg)); // payload may not be NULL-terminated
//     // printf("unyte_udp_get_payload_length: %u\n", unyte_udp_get_payload_length(seg));

//     /* Processing sample */
//     recv_count++;
//     print_udp_notif_header(seg->header, stdout);
//     // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);

//     fflush(stdout);

//     /* Struct frees */
//     unyte_udp_free_all(seg);
//   }

//   printf("Shutdown the socket\n");
//   close(*collector->sockfd);
//   pthread_join(*collector->main_thread, NULL);

//   // freeing collector mallocs
//   unyte_udp_free_collector(collector);
//   fflush(stdout);
//   return 0;
// }


/* server-dtls13-event.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *=============================================================================
 *
 * Single threaded example of a DTLS 1.3 server for instructional/learning
 * purposes. This example can handle multiple simultaneous connections by using
 * the libevent library to handle the event loop. Please note that this example
 * is not thread safe as access to global objects is not protected.
 *
 * Define USE_DTLS12 to use DTLS 1.2 instead of DTLS 1.3
 */

#include <wolfssl/options.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

/* Requires libevent */
#include <event2/event.h>

#include "../src/unyte_sender.h"
#include "../src/hexdump.h"

#define INVALID_SOCKET -1
#define MAXLINE   10000
#define QUICK_MULT  4               /* Our quick timeout multiplier */
#define CHGOODCB_E  (-1000)         /* An error outside the range of wolfSSL
                                     * errors */
#define CONN_TIMEOUT 1000             /* How long we wait for peer data before
                                     * closing the connection */

typedef struct conn_ctx {
    struct conn_ctx* next; //structure récursive
    WOLFSSL* ssl;
    struct event* readEv; //représentation d'un unique evenement
    struct event* writeEv; //An event can have some underlying condition it represents: a socket becoming readable or writeable (or both), or a signal becoming raised. (An event that represents no underlying condition is still useful: you can use one to implement a timer, or to communicate between threads.)
    char waitingOnData:1;
    int client_number; //print the number of the client connected who is currently talking on the connection, always incremented
} conn_ctx;

WOLFSSL_CTX*  ctx = NULL; //contexte dtls
struct event_base* base = NULL; //Structure to hold information and state for a Libevent dispatch loop.
WOLFSSL*      pendingSSL = NULL;
int           listenfd = INVALID_SOCKET;   /* Initialize our socket */
conn_ctx* active = NULL;
struct event* newConnEvent = NULL;
int client_number = 1;

struct thread_arguments{
    int client_number;
    int port_number;
    char * ip_address;
};

static void sig_handler(const int sig); //gestion des signaux
static void free_resources(void); //free des ressources allouées
static void newConn(evutil_socket_t fd, short events, void* arg); //acceptation de la connexion ssl (wolfssl_accept + gestion des erreurs)
static void dataReady(evutil_socket_t fd, short events, void* arg); //gestion des messages (ajouter les threads avant cette partie)
static int chGoodCb(WOLFSSL* ssl, void*); //check if everything with the connection is ok (fd, connect de la socket, création et gestion des events)
static int hsDoneCb(WOLFSSL* ssl, void*); //print les infos de la connexion ssl
static int newPendingSSL(struct thread_arguments * arguments); //définition et gestion de la connexion ssl (set_context + set_fd)
static int newFD(int port_number, char * ip_address); //création d'une nouvelle socket udp et est bind si tout est ok
static void conn_ctx_free(conn_ctx* connCtx); //free des ressources de la connexion conn_ctx



void* thread_function(void * args){

    pthread_detach(pthread_self());

    struct thread_arguments * args_to_send = (struct thread_arguments *)args;

    listenfd = newFD(args_to_send->port_number, args_to_send->ip_address);
    if (listenfd == INVALID_SOCKET)
        exit(EXIT_FAILURE);

    if (!newPendingSSL(args_to_send))
        exit(EXIT_FAILURE);

    signal(SIGINT, sig_handler);

    base = event_base_new();
    if (base == NULL) {
        perror("event_base_new failed");
        exit(EXIT_FAILURE);
    }

    newConnEvent = event_new(base, listenfd, EV_READ|EV_PERSIST, newConn, NULL);
    if (newConnEvent == NULL) {
        fprintf(stderr, "event_new failed for srvEvent\n");
        exit(EXIT_FAILURE);
    }
    if (event_add(newConnEvent, NULL) != 0) {
        fprintf(stderr, "event_add failed\n");
        exit(EXIT_FAILURE);
    }

    printf("running event loop\n");

    if (event_base_dispatch(base) == -1) {
        fprintf(stderr, "event_base_dispatch failed\n");
        exit(EXIT_FAILURE);
    }


    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 6){
        perror("Usage: ./client_sample <ip> <port> <ca_cert_name> <server_cert> <server_key_cert>");
        exit(1);
    }
    int exitVal = 1;
    int port_number_main = atoi(argv[2]);

    char * temp_certificats = "../certs/";
    char * caCertLoc_name = argv[3];
    char * servCertLoc_name = argv[4];
    char * servKeyLoc_name = argv[5];
    char caCertLoc[100];
    char servCertLoc[100];
    char servKeyLoc[100];

    strcpy(caCertLoc, temp_certificats);
    strcat(caCertLoc, caCertLoc_name);

    strcpy(servCertLoc, temp_certificats);
    strcat(servCertLoc, servCertLoc_name);

    strcpy(servKeyLoc, temp_certificats);
    strcat(servKeyLoc, servKeyLoc_name);

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
        free_resources();
        wolfSSL_Cleanup();
        return exitVal;
    }

    /* Load CA certificates */
    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", caCertLoc);
        free_resources();
        wolfSSL_Cleanup();
        return exitVal;
    }
    /* Load server certificates */
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servCertLoc);
        free_resources();
        wolfSSL_Cleanup();
        return exitVal;
    }
    /* Load server Keys */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servKeyLoc);
        free_resources();
        wolfSSL_Cleanup();
        return exitVal;
    }

    struct thread_arguments args;
    args.client_number = client_number;
    args.port_number = port_number_main;
    args.ip_address = argv[1];

    pthread_t thread;
    int res_thread = pthread_create(&thread, NULL, thread_function, &args);

    if (res_thread < 0){
        perror("Thread problem..");
        exit(EXIT_FAILURE);
    }

    pthread_join(thread, NULL);
    
    exitVal = 0; //everything is ok
    return exitVal;
}

static int newFD(int port_number, char * ip_address)
{
    int fd;
    int on = 1;
    struct sockaddr_in servAddr;        /* our server's address */

    /* Create a UDP/IP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket()");
        return INVALID_SOCKET;
    }
    memset((char *)&servAddr, 0, sizeof(servAddr));
    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servAddr.sin_family      = AF_INET;
    inet_aton(ip_address, &servAddr.sin_addr);
    servAddr.sin_port = htons(port_number);

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) != 0) {
        perror("setsockopt() with SO_REUSEADDR");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        return INVALID_SOCKET;
    }
#ifdef SO_REUSEPORT
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on)) != 0) {
        perror("setsockopt() with SO_REUSEPORT");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        return INVALID_SOCKET;
    }
#endif
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        return INVALID_SOCKET;
    }

    /* Bind Socket */
    if (bind(fd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind()");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        return INVALID_SOCKET;
    }
    return fd;  
}

static int newPendingSSL(struct thread_arguments * arguments)
{
    WOLFSSL* ssl;

    /* Create the pending WOLFSSL Object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        return 0;
    }

    wolfSSL_dtls_set_using_nonblock(ssl, 1); //This function informs the WOLFSSL DTLS object that the underlying UDP I/O is non-blocking. After an application creates a WOLFSSL object, if it will be used with a non-blocking UDP socket, call wolfSSL_dtls_set_using_nonblock() on it

    if (wolfDTLS_SetChGoodCb(ssl, chGoodCb, arguments) != WOLFSSL_SUCCESS ) { //pas de doc pour cette fonction
        fprintf(stderr, "wolfDTLS_SetChGoodCb error.\n");
        wolfSSL_free(ssl);
        return 0;
    }

    if (wolfSSL_SetHsDoneCb(ssl, hsDoneCb, NULL) != WOLFSSL_SUCCESS ) { //This function sets the handshake done callback. The hsDoneCb and hsDoneCtx members of the WOLFSSL structure are set in this function.
        //cb a function pointer of type HandShakeDoneCb with the signature of the form: int (HandShakeDoneCb)(WOLFSSL, void*);
        fprintf(stderr, "wolfSSL_SetHsDoneCb error.\n");
        wolfSSL_free(ssl);
        return 0;
    }

    if (wolfSSL_set_fd(ssl, listenfd) != WOLFSSL_SUCCESS) { //set la connexion ssl avec la socket créée
        fprintf(stderr, "wolfSSL_set_fd error.\n");
        wolfSSL_free(ssl);
        return 0;
    }

#if !defined(USE_DTLS12) && defined(WOLFSSL_SEND_HRR_COOKIE)
    {
        /* Applications should update this secret periodically */
        char *secret = "My secret";
        if (wolfSSL_send_hrr_cookie(ssl, (byte*)secret, strlen(secret)) //This function is called on the server side to indicate that a HelloRetryRequest message must contain a Cookie and, in case of using protocol DTLS v1.3, that the handshake will always include a cookie exchange. Please note that when using protocol DTLS v1.3, the cookie exchange is enabled by default. The Cookie holds a hash of the current transcript so that another server process can handle the ClientHello in reply. The secret is used when generting the integrity check on the Cookie data.
                != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_send_hrr_cookie error.\n");
            wolfSSL_free(ssl);
            return 0;
        }
    }
#endif

    pendingSSL = ssl;

    return 1;
}

static void newConn(evutil_socket_t fd, short events, void* arg) //no usage of the socket parameter
{
    int                ret;
    int                err;
    /* Store pointer because pendingSSL can be modified in chGoodCb */
    WOLFSSL*           ssl = pendingSSL;

    (void)events;
    (void)arg;

    ret = wolfSSL_accept(ssl);
    if (ret != WOLFSSL_SUCCESS) {
        err = wolfSSL_get_error(ssl, 0);
        if (err != WOLFSSL_ERROR_WANT_READ) {
            fprintf(stderr, "error = %d, %s\n", err,
                    wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "SSL_accept failed.\n");
            free_resources();
            wolfSSL_Cleanup();
            exit(1);
        }
    }
}

static void setHsTimeout(WOLFSSL* ssl, struct timeval *tv) //set d'un nouveau timeout pour la connexion ssl (voir grand car sinon le serveur ferme les connexions clients)
{
    int timeout = wolfSSL_dtls_get_current_timeout(ssl);
#ifndef USE_DTLS12
    if (wolfSSL_dtls13_use_quick_timeout(ssl)) {
        if (timeout >= QUICK_MULT)
            tv->tv_sec = timeout / QUICK_MULT;
        else
            tv->tv_usec = timeout * 1000000 / QUICK_MULT;
    }
    else
#endif
        tv->tv_sec = timeout;
}

/* Called when we have verified a connection */
static int chGoodCb(WOLFSSL* ssl, void* arg) //gestion de connexions clients
{
    int fd = INVALID_SOCKET;
    struct sockaddr_in cliaddr;         /* the client's address */
    socklen_t          cliLen = sizeof(cliaddr);
    conn_ctx* connCtx = (conn_ctx*)calloc(1, sizeof(conn_ctx));
    struct timeval tv;

    struct thread_arguments * arguments = arg;
    connCtx->client_number = arguments->client_number;
    int port = arguments->port_number;
    char * ip_address = arguments->ip_address;

    if (connCtx == NULL) {
        fprintf(stderr, "Out of memory!\n");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }

    connCtx->client_number = arguments->client_number; //we take the current value of the client
    arguments->client_number++; //we add +1 for the futur client connection identifiant, we go here for every single connection 

    /* Push to active connection stack */
    connCtx->next = active;
    active = connCtx;

    if (wolfSSL_dtls_get_peer(ssl, &cliaddr, &cliLen) != WOLFSSL_SUCCESS) { //get le peer à qui est connecté le client
        fprintf(stderr, "wolfSSL_dtls_get_peer failed\n");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }

    /* We need to change the SFD here so that the ssl object doesn't drop any
     * new connections */
    fd = newFD(port, ip_address);
    if (fd == INVALID_SOCKET){
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }

    /* Limit new SFD to only this connection */
    if (connect(fd, (const struct sockaddr*)&cliaddr, cliLen) != 0) { //connect de la socket
        perror("connect()");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }

    if (wolfSSL_set_dtls_fd_connected(ssl, fd) != WOLFSSL_SUCCESS) { //This function assigns a file descriptor (fd) as the input/output facility for the SSL connection. Typically this will be a socket file descriptor. This is a DTLS specific API because it marks that the socket is connected. recvfrom and sendto calls on this fd will have the addr and addr_len parameters set to NULL.
        fprintf(stderr, "wolfSSL_set_dtls_fd_connected error.\n");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }

    connCtx->writeEv = event_new(base, fd, EV_WRITE, dataReady, connCtx); //Allocate and assign a new event structure, ready to be added.
    //The function event_new() returns a new event that can be used in future calls to event_add() and event_del(). The fd and events arguments determine which conditions will trigger the event; the callback and callback_arg arguments tell Libevent what to do when the event becomes active.
    if (connCtx->writeEv == NULL) {
        fprintf(stderr, "event_new failed for srvEvent\n");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }
    connCtx->readEv = event_new(base, fd, EV_READ, dataReady, connCtx);
    if (connCtx->readEv == NULL) {
        fprintf(stderr, "event_new failed for srvEvent\n");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }
    memset(&tv, 0, sizeof(tv));
    setHsTimeout(ssl, &tv);
    /* We are using non-blocking sockets so we will definitely be waiting for
     * the peer. Start the timer now. */
    if (event_add(connCtx->readEv, &tv) != 0) { //Add an event to the set of pending events.
    //The function event_add() schedules the execution of the event 'ev' when the condition specified by event_assign() or event_new() occurs, or when the time specified in timeout has elapsed. If a timeout is NULL, no timeout occurs and the function will only be called if a matching event occurs. The event in the ev argument must be already initialized by event_assign() or event_new() and may not be used in calls to event_assign() until it is no longer pending.
        fprintf(stderr, "event_add failed\n");
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }

    /* Promote the pending connection to an active connection */
    if (!newPendingSSL(arguments)){
        if (fd != INVALID_SOCKET) {
            close(fd);
            fd = INVALID_SOCKET;
        }
        if (connCtx != NULL) {
            connCtx->ssl = NULL;
            conn_ctx_free(connCtx);
        }
        (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
        return CHGOODCB_E;
    }
    connCtx->ssl = ssl;

    return 0;
}

static int hsDoneCb(WOLFSSL* ssl, void* arg) //print les infos de la connexion 
{
    printf("New connection established using %s %s\n", wolfSSL_get_version(ssl), wolfSSL_get_cipher(ssl));
    (void)arg;
    return 0;
}

static void dataReady(evutil_socket_t fd, short events, void* arg) //lecture et écriture des données envoyées et reçues
{
    conn_ctx* connCtx = (conn_ctx*)arg;
    int client_number = connCtx->client_number;
    int ret;
    int err;
    struct timeval tv;
    char msg[MAXLINE];
    int msgSz;
    const char* ack = "I hear you fashizzle!\n";

    memset(&tv, 0, sizeof(tv));
    if (events & EV_TIMEOUT) {
        /* A timeout occurred */
        if (!wolfSSL_is_init_finished(connCtx->ssl)) { //This function checks to see if the connection is established.
            if (wolfSSL_dtls_got_timeout(connCtx->ssl) != WOLFSSL_SUCCESS) { //When using non-blocking sockets with DTLS, this function should be called on the WOLFSSL object when the controlling code thinks the transmission has timed out. It performs the actions needed to retry the last transmit, including adjusting the timeout value. If it has been too long, this will return a failure.
                fprintf(stderr, "wolfSSL_dtls_got_timeout failed\n");
                conn_ctx_free(connCtx);
                close(fd);
            }
            setHsTimeout(connCtx->ssl, &tv);
            if (event_add(connCtx->readEv, &tv) != 0) {
                fprintf(stderr, "event_add failed\n");
                conn_ctx_free(connCtx);
                close(fd);
            }
        }
        else {
            if (connCtx->waitingOnData) {
                /* Too long waiting for peer data. Shutdown the connection.
                 * Don't wait for a response from the peer. */
                printf("Closing connection after timeout\n");
                (void)wolfSSL_shutdown(connCtx->ssl); //shutdown de la connexion ssl
                conn_ctx_free(connCtx);
                close(fd);
            }
            else {
                tv.tv_sec = CONN_TIMEOUT;
                connCtx->waitingOnData = 1;
                if (event_add(connCtx->readEv, &tv) != 0) {
                    fprintf(stderr, "event_add failed\n");
                    conn_ctx_free(connCtx);
                    close(fd);
                }
            }
        }
    }
    else if (events & (EV_READ|EV_WRITE)) {
        ret = wolfSSL_read(connCtx->ssl, msg, sizeof(msg) - 1);
        if (ret > 0) {
            msgSz = ret;
            msg[msgSz] = '\0';
            printf("Received message from client %d :\n", client_number);
            hexdump(msg, msgSz); //pas d'affichage correct avec la taille spécifiée
            ret = wolfSSL_write(connCtx->ssl, ack, strlen(ack));
        }

        if (ret <= 0) {
            err = wolfSSL_get_error(connCtx->ssl, 0);
            if (err == WOLFSSL_ERROR_WANT_READ ||
                    err == WOLFSSL_ERROR_WANT_WRITE) {
                setHsTimeout(connCtx->ssl, &tv);
                if (event_add(err == WOLFSSL_ERROR_WANT_READ ?
                        connCtx->readEv : connCtx->writeEv, &tv) != 0) {
                    fprintf(stderr, "event_add failed\n");
                    conn_ctx_free(connCtx);
                    close(fd);
                }
            }
            else if (err == WOLFSSL_ERROR_ZERO_RETURN) {
                /* Peer closed connection. Let's do the same. */
                printf("peer closed connection\n");
                ret = wolfSSL_shutdown(connCtx->ssl);
                if (ret != WOLFSSL_SUCCESS) {
                    fprintf(stderr, "wolfSSL_shutdown failed (%d)\n", ret);
                }
                conn_ctx_free(connCtx);
                close(fd);
            }
            else {
                fprintf(stderr, "error = %d, %s\n", err,
                        wolfSSL_ERR_reason_error_string(err));
                fprintf(stderr, "wolfSSL_read or wolfSSL_write failed\n");
                conn_ctx_free(connCtx);
                close(fd);
            }
        }
        else {
            tv.tv_sec = CONN_TIMEOUT;
            connCtx->waitingOnData = 1;
            if (event_add(connCtx->readEv, &tv) != 0) {
                fprintf(stderr, "event_add failed\n");
                conn_ctx_free(connCtx);
                close(fd);
            }
        }
    }
    else {
        fprintf(stderr, "Unexpected events %d\n", events);
        conn_ctx_free(connCtx);
        close(fd);
    }


    return;
}

static void conn_ctx_free(conn_ctx* connCtx) //libération des ressources de la connexion
{
    if (connCtx != NULL) {
        /* Remove from active stack */
        if (active != NULL) {
            conn_ctx** prev = &active;
            while (*prev != NULL) {
                if (*prev == connCtx) {
                    *prev = connCtx->next;
                    break;
                }
                prev = &(*prev)->next;
            }
        }
        if (connCtx->ssl != NULL)
            wolfSSL_free(connCtx->ssl);
        if (connCtx->readEv != NULL) {
            (void)event_del(connCtx->readEv);
            event_free(connCtx->readEv);
        }
        if (connCtx->writeEv != NULL) {
            (void)event_del(connCtx->writeEv);
            event_free(connCtx->writeEv);
        }
        free(connCtx);
    }
}

static void sig_handler(const int sig) //gestion du ctrl+c
{
    printf("Received signal %d. Cleaning up.\n", sig);
    free_resources();
    wolfSSL_Cleanup();
    exit(0);
}

static void free_resources(void) //libération des ressources
{
    conn_ctx* connCtx = active;
    while (connCtx != NULL) {
        active = active->next;
        conn_ctx_free(connCtx);
        connCtx = active;
    }
    if (pendingSSL != NULL) {
        wolfSSL_shutdown(pendingSSL);
        wolfSSL_free(pendingSSL);
        pendingSSL = NULL;
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (listenfd != INVALID_SOCKET) {
        close(listenfd);
        listenfd = INVALID_SOCKET;
    }
    if (newConnEvent != NULL) {
        (void)event_del(newConnEvent);
        event_free(newConnEvent);
        newConnEvent = NULL;
    }
    if (base != NULL) {
        event_base_free(base);
        base = NULL;
    }
}