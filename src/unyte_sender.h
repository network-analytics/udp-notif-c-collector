#ifndef H_UNYTE_SENDER
#define H_UNYTE_SENDER

#include <stdint.h>
#include "unyte_udp_utils.h"

//dtls header
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include "dtls-common.h"


#define DEFAULT_MTU 1500
#define DEFAULT_SK_SND_BUFF_SIZE 20971520 // 20MB of socket buffer size

typedef struct
{
  char *address;
  char *port;
  uint default_mtu;
  char *interface;
  uint64_t socket_buff_size;  // socket buffer size in bytes
} unyte_sender_options_t;

//#ifdef ENABLE_DTLS_
typedef struct
{
    WOLFSSL* ssl;
    WOLFSSL_CTX* ctx;
} dtls_params;
//#endif

struct unyte_sender_socket
{
  int sockfd;
  struct sockaddr_storage *sock_in;
  uint default_mtu;
//#ifdef ENABLE_DTLS_
    dtls_params dtls;
//#endif
};


/**
 * Initializes the sender to address and port set in options
 */
struct unyte_sender_socket *unyte_start_sender(unyte_sender_options_t *options);

/**
 * Sends message to unyte socket
 */
int unyte_send(struct unyte_sender_socket *sender_sk, unyte_message_t *message);
int unyte_send_with_context(struct unyte_sender_socket *sender_sk, unyte_message_t *message, int reuse);

/**
 * Free unyte socket sender struct
 */
int free_sender_socket(struct unyte_sender_socket *sender_sk);

/**
 * Free segmented messages after sent
 */
int free_seg_msgs(struct unyte_segmented_msg *packets);
int free_unyte_sent_message(unyte_message_t *msg);

#endif
