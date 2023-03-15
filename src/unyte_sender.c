#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <net/if.h>
#include <netdb.h>
#include "unyte_sender.h"

int cleanup_main(WOLFSSL * ssl, int sockfd, WOLFSSL_CTX * ctx){
    if (ssl != NULL) {
        /* Attempt a full shutdown */
        int ret = wolfSSL_shutdown(ssl);
        if (ret == WOLFSSL_SHUTDOWN_NOT_DONE)
            ret = wolfSSL_shutdown(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            int err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "wolfSSL_shutdown failed\n");
        }
        wolfSSL_free(ssl);
    }
    if (sockfd != INVALID_SOCKET)
        close(sockfd);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 1;
}


struct unyte_sender_socket *unyte_start_sender_dtls(unyte_sender_options_t *options){

  struct dtls_params *dtls = (struct dtls_params *)malloc(sizeof(struct dtls_params));


//start dtls -----------------------
  WOLFSSL * ssl = NULL;
  WOLFSSL_CTX * ctx = NULL;
  int sockfd = INVALID_SOCKET;

  if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
      fprintf(stderr, "wolfSSL_CTX_new error.\n");
      cleanup_main(ssl, sockfd, ctx);
  }

  //wolfSSL_Debugging_ON();

  if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_3_client_method())) == NULL) {
      fprintf(stderr, "wolfSSL_CTX_new error.\n");
      cleanup_main(ssl, sockfd, ctx);
  }

  if (wolfSSL_CTX_load_verify_locations(ctx, caCertLoc, 0) != SSL_SUCCESS) {
      fprintf(stderr, "Error loading %s, please check the file.\n", caCertLoc);
      cleanup_main(ssl, sockfd, ctx);
  }

  ssl = wolfSSL_new(ctx);
  if (ssl == NULL) {
      fprintf(stderr, "unable to get ssl object\n");
      cleanup_main(ssl, sockfd, ctx);
  }
  //end dtls -----------------------

  struct unyte_sender_socket *sender_sk = (struct unyte_sender_socket *)malloc(sizeof(struct unyte_sender_socket));
  struct sockaddr_in address;

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(options->port));
  if (inet_pton(AF_INET, options->address, &address.sin_addr) < 1) {
      perror("inet_pton()");
      cleanup_main(ssl, sockfd, ctx);
  }

  if (sender_sk == NULL){
    perror("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  //start dtls -----------------------
  if (wolfSSL_dtls_set_peer(ssl, &address, sizeof(address)) != WOLFSSL_SUCCESS) {
      fprintf(stderr, "wolfSSL_dtls_set_peer failed\n");
      cleanup_main(ssl, sockfd, ctx);
  }
  //end dtls -----------------------


  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0)
  {
    perror("socket()");
    cleanup_main(ssl, sockfd, ctx);
    exit(EXIT_FAILURE);
  }

  int check_non_blocking = wolfSSL_get_using_nonblock(ssl);
  if (check_non_blocking){
    printf("non blocking\n");
  } else {
    printf("blocking\n");
  }


  //start dtls -----------------------
  if (wolfSSL_set_fd(ssl, sockfd) != WOLFSSL_SUCCESS) {
      fprintf(stderr, "cannot set socket file descriptor\n");
      cleanup_main(ssl, sockfd, ctx);
  }

  if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
      int err = wolfSSL_get_error(ssl, 0);
      fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
      fprintf(stderr, "wolfSSL_connect failed\n");
      cleanup_main(ssl, sockfd, ctx);
  }
  //end dtls -----------------------

  uint64_t send_buf_size = DEFAULT_SK_SND_BUFF_SIZE;
  if (options->socket_buff_size > 0)
    send_buf_size = options->socket_buff_size;

  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size)))
  {
    perror("Set socket buffer size");
    exit(EXIT_FAILURE);
  }


  sender_sk->sock_in = &address;
  sender_sk->sockfd = sockfd;
  sender_sk->default_mtu = options->default_mtu;
  dtls->ssl = ssl;
  dtls->ctx = ctx;
  sender_sk->dtls = dtls;
  return sender_sk;
}

struct unyte_sender_socket *unyte_start_sender(unyte_sender_options_t *options)
{
  struct addrinfo *addr_info;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

  int rc = getaddrinfo(options->address, options->port, &hints, &addr_info);

  if (rc != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(rc));
    exit(EXIT_FAILURE);
  }

  struct unyte_sender_socket *sender_sk = (struct unyte_sender_socket *)malloc(sizeof(struct unyte_sender_socket));
  struct sockaddr_storage *address = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));

  if (address == NULL || sender_sk == NULL){
    perror("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  // save copy of the connected addr
  memset(address, 0, sizeof(*address));
  memcpy(address, addr_info->ai_addr, addr_info->ai_addrlen);

  int sockfd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);

  if (sockfd < 0)
  {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  // Binding socket to an interface
  const char *interface = options->interface;
  if (options->interface != NULL && (strlen(interface) > 0))
  {
    printf("Setting interface: %s\n", interface);
    int len = strnlen(interface, IFNAMSIZ);
    if (len == IFNAMSIZ)
    {
      fprintf(stderr, "Too long iface name");
      exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface, len) < 0)
    {
      perror("Bind socket to interface failed");
      exit(EXIT_FAILURE);
    }
  }

  uint64_t send_buf_size = DEFAULT_SK_SND_BUFF_SIZE;
  if (options->socket_buff_size > 0)
    send_buf_size = options->socket_buff_size;

  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size)))
  {
    perror("Set socket buffer size");
    exit(EXIT_FAILURE);
  }

  // connect socket to destination address
  if (connect(sockfd, addr_info->ai_addr, (int)addr_info->ai_addrlen) == -1)
  {
    perror("Connect failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // free addr_info after usage
  freeaddrinfo(addr_info);

  sender_sk->sock_in = address;
  sender_sk->sockfd = sockfd;
  sender_sk->default_mtu = options->default_mtu;
  return sender_sk;
}

int unyte_send(struct unyte_sender_socket *sender_sk, unyte_message_t *message)
{
  uint mtu = message->used_mtu;
  if (mtu <= 0)
  {
    if (sender_sk->default_mtu > 0)
      mtu = sender_sk->default_mtu;
    else
      mtu = DEFAULT_MTU;
  }

  struct unyte_segmented_msg *packets = build_message(message, mtu);
  unyte_seg_met_t *current_seg = packets->segments;
  for (uint i = 0; i < packets->segments_len; i++)
  {
    unsigned char *parsed_packet = serialize_message(current_seg);
    int res_send = send(sender_sk->sockfd, parsed_packet, current_seg->header->header_length + current_seg->header->message_length, 0);

    if (res_send < 0)
    {
      // perror("send()");
    }
    free(parsed_packet);
    current_seg++;
  }
  free_seg_msgs(packets);

  return 0;
}

int unyte_send_dtls(struct unyte_sender_socket *sender_sk, unyte_message_t *message)
{
  char recvLine[MAXLINE - 1]; //just to check if we have a response
  int err, n;
  uint mtu = message->used_mtu;
  if (mtu <= 0)
  {
    if (sender_sk->default_mtu > 0)
      mtu = sender_sk->default_mtu;
    else
      mtu = DEFAULT_MTU;
  }

  //start dtls -----------------------
  //showConnInfo(sender_sk->dtls->ssl);
  //end dtls -----------------------


  struct unyte_segmented_msg *packets = build_message(message, mtu);
  unyte_seg_met_t *current_seg = packets->segments;

  for (uint i = 0; i < packets->segments_len; i++)
  {
    unsigned char *parsed_packet = serialize_message(current_seg);

    //start dtls -----------------------
    if (wolfSSL_write(sender_sk->dtls->ssl, parsed_packet, strlen(parsed_packet)) != strlen(parsed_packet)) {
        err = wolfSSL_get_error(sender_sk->dtls->ssl, 0);
        fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
        fprintf(stderr, "wolfSSL_write failed\n");
        cleanup_main(sender_sk->dtls->ssl, sender_sk->sockfd, sender_sk->dtls->ctx);
    }

    n = wolfSSL_read(sender_sk->dtls->ssl, recvLine, sizeof(recvLine)-1);

    if (n > 0) {
        recvLine[n] = '\0';
        printf("%s\n", recvLine);
    }
    else {
        err = wolfSSL_get_error(sender_sk->dtls->ssl, 0);
        fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
        fprintf(stderr, "wolfSSL_read failed\n");
        cleanup_main(sender_sk->dtls->ssl, sender_sk->sockfd, sender_sk->dtls->ctx);
    }
    //end dtls -----------------------

    free(parsed_packet);
    current_seg++;
  }
  free_seg_msgs(packets);

  return 0;
}

int free_sender_socket(struct unyte_sender_socket *sender_sk)
{
  //free(sender_sk->sock_in);
  free(sender_sk->dtls); //add macro here
  free(sender_sk);
  return 0;
}

int free_seg_msgs(struct unyte_segmented_msg *packets)
{
  unyte_seg_met_t *current = packets->segments;
  for (uint i = 0; i < packets->segments_len; i++)
  {
    free(current->payload);
    unyte_option_t *next = current->header->options->next;
    while (next != NULL)
    {
      unyte_option_t *cur = next;
      next = next->next;
      free(cur->data);
      free(cur);
    }
    free(current->header->options);
    free(current->header);
    current++;
  }
  free(packets->segments);
  free(packets);
  return 0;
}

int free_unyte_sent_message(unyte_message_t *msg)
{
  free(msg->options);
  free(msg);
  return 0;
}
