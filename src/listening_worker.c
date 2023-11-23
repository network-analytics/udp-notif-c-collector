#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include "listening_worker.h"
#include "segmentation_buffer.h"
#include "parsing_worker.h"
#include "unyte_udp_collector.h"
#include "cleanup_worker.h"
#include "unyte_udp_defaults.h"

#include <netdb.h>
#include <wolfssl/error-ssl.h>
#include <signal.h>
#include <event2/event.h>
#include "../src/unyte_sender.h"
#include "../src/hexdump.h"

#define QUICK_MULT  4
#define CHGOODCB_E  (-1000)
#define CONN_TIMEOUT 1000 
#define MAXLINE   4096
#define INVALID_SOCKET -1
#define MAX_TO_RECEIVE 10

void stop_parsers_and_monitoring(struct parse_worker *parsers, struct listener_thread_input *in, struct monitoring_worker *monitoring)
{
  for (uint i = 0; i < in->nb_parsers; i++)
  {
    // stopping all clean up thread first
    (parsers + i)->cleanup_worker->cleanup_in->stop_cleanup_thread = 1;
  }
  monitoring->monitoring_in->stop_monitoring_thread = true;
}

/**
 * Frees all parsers mallocs and thread input memory.
 */
void free_parsers(struct parse_worker *parsers, struct listener_thread_input *in)
{
  /* Kill every workers here */
  for (uint i = 0; i < in->nb_parsers; i++)
  {
    // Kill clean up thread
    pthread_join(*(parsers + i)->cleanup_worker->cleanup_thread, NULL);
    free((parsers + i)->cleanup_worker->cleanup_thread);
    free((parsers + i)->cleanup_worker);

    // Kill worker thread
    pthread_cancel(*(parsers + i)->worker);
    pthread_join(*(parsers + i)->worker, NULL);
    free((parsers + i)->queue->data);
    free((parsers + i)->worker);
    free((parsers + i)->queue);
    clear_buffer((parsers + i)->input->segment_buff);
    free((parsers + i)->input);
  }

  free(parsers);

  free(in->conn->sockfd);
  free(in->conn->addr);
  free(in->conn);
}

void free_monitoring_worker(struct monitoring_worker *monitoring)
{
  if (monitoring->running)
    pthread_join(*monitoring->monitoring_thread, NULL);
  free(monitoring->monitoring_thread);
  unyte_udp_free_seg_counters(monitoring->monitoring_in->counters, monitoring->monitoring_in->nb_counters);
  free(monitoring->monitoring_in);
  free(monitoring);
}

/**
 * Creates a thread with a cleanup cron worker
 */
int create_cleanup_thread(struct segment_buffer *seg_buff, struct parse_worker *parser)
{
  pthread_t *clean_up_thread = (pthread_t *)malloc(sizeof(pthread_t));
  struct cleanup_thread_input *cleanup_in = (struct cleanup_thread_input *)malloc(sizeof(struct cleanup_thread_input));
  parser->cleanup_worker = (struct cleanup_worker *)malloc(sizeof(struct cleanup_worker));

  if (clean_up_thread == NULL || cleanup_in == NULL || parser->cleanup_worker == NULL)
  {
    printf("Malloc failed.\n");
    return -1;
  }

  cleanup_in->seg_buff = seg_buff;
  cleanup_in->time = CLEANUP_FLAG_CRON;
  cleanup_in->stop_cleanup_thread = 0;

  pthread_create(clean_up_thread, NULL, t_clean_up, (void *)cleanup_in);

  // Saving cleanup_worker references for free afterwards
  parser->cleanup_worker->cleanup_thread = clean_up_thread;
  parser->cleanup_worker->cleanup_in = cleanup_in;

  return 0;
}
/**
 * Creates a thread with a parse worker.
 */
int create_parse_worker(struct parse_worker *parser, struct listener_thread_input *in, unyte_seg_counters_t *counters, int monitoring_running)
{
  parser->queue = unyte_udp_queue_init(in->parser_queue_size);
  if (parser->queue == NULL)
  {
    // malloc failed
    return -1;
  }

  parser->worker = (pthread_t *)malloc(sizeof(pthread_t));
  // Define the struct passed to the next parser
  struct parser_thread_input *parser_input = (struct parser_thread_input *)malloc(sizeof(struct parser_thread_input));
  if (parser->worker == NULL || parser_input == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  /* Fill the struct */
  parser_input->input = parser->queue;
  parser_input->output = in->output_queue;
  parser_input->segment_buff = create_segment_buffer();
  parser_input->counters = counters;
  parser_input->counters->type = PARSER_WORKER;
  parser_input->legacy_proto = in->legacy_proto;

  for (uint i = 0; i < ACTIVE_ODIDS; i++)
  {
    (parser_input->counters->active_odids + i)->active = 0;
  }

  parser_input->monitoring_running = monitoring_running;

  if (parser_input->segment_buff == NULL || parser_input->counters->active_odids == NULL)
  {
    printf("Create segment buffer failed.\n");
    return -1;
  }

  /* Create the thread */
  pthread_create(parser->worker, NULL, t_parser, (void *)parser_input);

  /* Store the pointer to be able to free it at the end */
  parser->input = parser_input;

  return create_cleanup_thread(parser_input->segment_buff, parser);
}

int create_monitoring_thread(struct monitoring_worker *monitoring, unyte_udp_queue_t *out_mnt_queue, uint delay, unyte_seg_counters_t *counters, uint nb_counters)
{
  struct monitoring_thread_input *mnt_input = (struct monitoring_thread_input *)malloc(sizeof(struct monitoring_thread_input));

  pthread_t *th_monitoring = (pthread_t *)malloc(sizeof(pthread_t));

  if (mnt_input == NULL || th_monitoring == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  mnt_input->delay = delay;
  mnt_input->output_queue = out_mnt_queue;
  mnt_input->counters = counters;
  mnt_input->nb_counters = nb_counters;
  mnt_input->stop_monitoring_thread = false;

  if (out_mnt_queue->size != 0)
  {
    /* Create the thread */
    pthread_create(th_monitoring, NULL, t_monitoring_unyte_udp, (void *)mnt_input);
    monitoring->running = true;
  }
  else
  {
    monitoring->running = false;
  }

  /* Store the pointer to be able to free it at the end */
  monitoring->monitoring_in = mnt_input;
  monitoring->monitoring_thread = th_monitoring;

  return 0;
}

struct sockaddr_storage *get_dest_addr(struct msghdr *mh, unyte_udp_sock_t *sock)
{
  struct sockaddr_storage *addr = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));

  if (addr == NULL)
  {
    printf("Malloc failed\n");
    return NULL;
  }

  memset(addr, 0, sizeof(struct sockaddr_storage));
  struct in_pktinfo *in_pktinfo;
  struct in6_pktinfo *in6_pktinfo;

  for ( // iterate through all the control headers
      struct cmsghdr *cmsg = CMSG_FIRSTHDR(mh);
      cmsg != NULL;
      cmsg = CMSG_NXTHDR(mh, cmsg))
  {
    if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO)
    {
      in_pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsg);
      ((struct sockaddr_in *)addr)->sin_family = AF_INET;
      ((struct sockaddr_in *)addr)->sin_addr = in_pktinfo->ipi_addr;
      ((struct sockaddr_in *)addr)->sin_port = ((struct sockaddr_in *)sock->addr)->sin_port;
      break;
    }
    if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO)
    {
      in6_pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
      ((struct sockaddr_in6 *)addr)->sin6_family = AF_INET6;
      ((struct sockaddr_in6 *)addr)->sin6_addr = in6_pktinfo->ipi6_addr;
      ((struct sockaddr_in6 *)addr)->sin6_port = ((struct sockaddr_in6 *)sock->addr)->sin6_port;
      break;
    }
  }
  return addr;
}




//DTLS integration

typedef struct conn_ctx {
    struct conn_ctx* next; //structure récursive
    WOLFSSL* ssl;
    struct event* readEv; //représentation d'un unique evenement
    struct event* writeEv; //An event can have some underlying condition it represents: a socket becoming readable or writeable (or both), or a signal becoming raised. (An event that represents no underlying condition is still useful: you can use one to implement a timer, or to communicate between threads.)
    char waitingOnData:1;
    int client_number; //print the number of the client connected who is currently talking on the connection, always incremented
    struct parse_worker * parsers;
    struct monitoring_worker * monitoring;
    struct listener_thread_input *in;
    char * ip_dest_address;
    struct sockaddr_in cliaddr;
    int port;
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
    struct parse_worker * parsers;
    struct monitoring_worker * monitoring;
    struct listener_thread_input *in;
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


int dtls_server_launcher(int fd, char * ip_address, int port, char * cacert, char * servercert, char * serverkey, struct parse_worker * parsers, struct monitoring_worker * monitoring, struct listener_thread_input *in)
{
    int exitVal = 1;

    char * temp_certificats = "../certs/";

    char caCertLoc[100];
    char servCertLoc[100];
    char servKeyLoc[100];

    strcpy(caCertLoc, temp_certificats);
    strcat(caCertLoc, cacert);

    strcpy(servCertLoc, temp_certificats);
    strcat(servCertLoc, servercert);

    strcpy(servKeyLoc, temp_certificats);
    strcat(servKeyLoc, serverkey);

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
    args.client_number = 1;
    args.port_number = port;
    args.ip_address = ip_address;
    args.parsers = parsers;
    args.monitoring = monitoring;
    args.in = in;

    listenfd = fd;
    if (listenfd == INVALID_SOCKET)
        exit(EXIT_FAILURE);

    if (!newPendingSSL(&args))
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

    if (event_base_dispatch(base) == -1) {
        fprintf(stderr, "event_base_dispatch failed\n");
        exit(EXIT_FAILURE);
    }

    return 0;
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

static void newConn(evutil_socket_t fd, short events, void* arg) //no usage of the socket parameter / do the wolfss_accept on the connection
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
    connCtx->parsers = arguments->parsers;
    connCtx->monitoring = arguments->monitoring;
    connCtx->in = arguments->in;
    connCtx->ip_dest_address = arguments->ip_address;
    connCtx->port = arguments->port_number;
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

    connCtx->cliaddr = cliaddr;

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
    int msgSz;

    struct parse_worker * parsers = connCtx->parsers;
    struct monitoring_worker * monitoring = connCtx->monitoring;
    struct listener_thread_input *in = connCtx->in;

    unyte_seg_counters_t *listener_counter = monitoring->monitoring_in->counters + in->nb_parsers;
    listener_counter->thread_id = pthread_self();
    listener_counter->type = LISTENER_WORKER;

    // char ip_source[200];
    // inet_ntop(AF_INET, &(connCtx->cliaddr.sin_addr), ip_source, INET_ADDRSTRLEN);
    // printf("ip client %s\n", ip_source);

    struct sockaddr_storage source;
    memcpy(&source, &(connCtx->cliaddr), sizeof(struct sockaddr_in));

    struct sockaddr_storage destination;
    struct sockaddr_in * ip_temp = (struct sockaddr_in *)&destination;

    ip_temp->sin_family = AF_INET;
    inet_pton(AF_INET, connCtx->ip_dest_address, &(ip_temp->sin_addr));
    ip_temp->sin_port = connCtx->port;


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
    else if (events & EV_READ) {

        char * msg = malloc(UDP_SIZE * sizeof(char));
        ret = wolfSSL_read(connCtx->ssl, msg, UDP_SIZE);

        if (ret > 0) {
            msgSz = ret;
            // printf("\nReceived message from client %d :\n", client_number);
            // hexdump(msg, msgSz); //pas d'affichage correct avec la taille spécifiée

            // printf("version = %s\n", wolfSSL_get_version(connCtx->ssl));
            // printf("ip dest address = %s\n", connCtx->ip_dest_address);
            // printf("ip source address = %s\n", connCtx->ip_source_address); 


            unyte_min_t *seg = minimal_parse(msg, &source, &destination);

            if (seg == NULL)
            {
                printf("minimal_parse error\n");
                return;
            }

            uint32_t seg_odid = seg->observation_domain_id;
            uint32_t seg_mid = seg->message_id;
            int ret = unyte_udp_queue_write((parsers + (seg->observation_domain_id % in->nb_parsers))->queue, seg);
            // if ret == -1 --> queue is full, we discard message
            if (ret < 0)
            {
              // printf("1.losing message on parser queue\n");
              if (monitoring->running)
                unyte_udp_update_dropped_segment(listener_counter, seg_odid, seg_mid);
              free(seg->buffer);
              free(seg);
            }
            else if (monitoring->running)
              unyte_udp_update_received_segment(listener_counter, seg_odid, seg_mid);

            tv.tv_sec = CONN_TIMEOUT;
            connCtx->waitingOnData = 1;
            if (event_add(connCtx->readEv, &tv) != 0) {
                fprintf(stderr, "event_add failed\n");
                conn_ctx_free(connCtx);
                close(fd);
            }

        } else {
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
        stop_parsers_and_monitoring(connCtx->parsers, connCtx->in, connCtx->monitoring);
        free_parsers(connCtx->parsers, connCtx->in);
        free_monitoring_worker(connCtx->monitoring);
        close(*(connCtx->in->conn->sockfd));
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

/**
 * Udp listener worker on PORT port.
 */
int listener(struct listener_thread_input *in)
{
  // Create parsing workers and monitoring worker
  struct parse_worker *parsers = malloc(sizeof(struct parse_worker) * in->nb_parsers);
  struct monitoring_worker *monitoring = malloc(sizeof(struct monitoring_worker));

  if (parsers == NULL || monitoring == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  uint nb_counters = in->nb_parsers + 1;
  unyte_seg_counters_t *counters = unyte_udp_init_counters(nb_counters); // parsers + listening workers
  if (parsers == NULL || monitoring == NULL || counters == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  int monitoring_ret = create_monitoring_thread(monitoring, in->monitoring_queue, in->monitoring_delay, counters, nb_counters);
  if (monitoring_ret < 0)
    return monitoring_ret;

  for (uint i = 0; i < in->nb_parsers; i++)
  {
    int created = create_parse_worker((parsers + i), in, (monitoring->monitoring_in->counters + i), monitoring->running);
    if (created < 0)
      return created;
  }

    int dtls = dtls_server_launcher(in->sockfd, in->ip, in->port, in->cacert, in->servercert, in->serverkey, parsers, monitoring, in);
    if (dtls < 0){ 
        return dtls;
    }

  return 0;
}

/**
 * Threadified app functin listening on *P port.
 */
void *t_listener(void *in)
{
  listener((struct listener_thread_input *)in);
  printf("t_listener\n");
  free(in);
  pthread_exit(NULL);
}