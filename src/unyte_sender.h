// #include <pthread.h>
#include <stdint.h>
#include "unyte_utils.h"

#ifndef H_UNYTE_SENDER
#define H_UNYTE_SENDER

#define DEFAULT_MTU 1500

typedef struct
{
  char *address;
  uint16_t port;
  uint default_mtu;
} unyte_sender_options_t;

struct unyte_sender_socket
{
  int sockfd;
  struct sockaddr_in *sock_in;
  uint default_mtu;
};

struct unyte_sender_socket *unyte_start_sender(unyte_sender_options_t *options);
int unyte_send(struct unyte_sender_socket *sender_sk, unyte_message_t *message);
int free_sender_socket(struct unyte_sender_socket *sender_sk);

#endif
