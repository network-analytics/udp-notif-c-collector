#include <stdint.h>
#include "queue.h"
#include "unyte_utils.h"

#ifndef H_LISTENING_WORKER
#define H_LISTENING_WORKER

#define RCVSIZE 65535
#define QUEUE_SIZE 50
#define PARSER_NUMBER 10
#define CLEANUP_FLAG_CRON 1000

/**
 * Input given to the listener thread when creating it.
 */
struct listener_thread_input
{
  queue_t *output_queue;  /* The queue used to push the segments outside. */
  uint16_t port;          /* The port to initialize the interface on. */
  unyte_sock_t *conn;     /* Connection with addr, sockfd */
  uint16_t recvmmsg_vlen; /* The recvmmsg buffer array size */
};

struct parse_worker
{
  queue_t *queue;
  pthread_t *worker;
  // pthread_t *cleanup_thread;
  struct parser_thread_input *input;
  struct cleanup_worker *cleanup_worker;
};

struct cleanup_worker
{
  pthread_t *cleanup_thread;
  struct cleanup_thread_input *cleanup_in;
  // uint8_t stop_thread;
};

int listener(struct listener_thread_input *in);
void *t_listener(void *in);
// int create_cleanup_thread(struct segment_buffer *seg_buff, struct parse_worker *parser);

#endif
