#ifndef H_UNYTE_SENDER
#define H_UNYTE_SENDER

#include <stdint.h>
#include "unyte_utils.h"

#define DEFAULT_MTU 1500

typedef struct
{
  char *address;
  uint16_t port;
  uint default_mtu;
  char *interface;
} unyte_sender_options_t;

struct unyte_sender_socket
{
  int sockfd;
  struct sockaddr_in *sock_in;
  uint default_mtu;
};

/**
 * Initializes the sender to address and port set in options
 */
struct unyte_sender_socket *unyte_start_sender(unyte_sender_options_t *options);

/**
 * Sends message to unyte socket
 */
int unyte_send(struct unyte_sender_socket *sender_sk, unyte_message_t *message);

/**
 * Free unyte socket sender struct
 */
int free_sender_socket(struct unyte_sender_socket *sender_sk);

/**
 * Free segmented messages after sent
 */
int free_seg_msgs(struct unyte_segmented_msg *packets);

#endif
