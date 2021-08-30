#ifndef H_LISTENING_WORKER
#define H_LISTENING_WORKER

#include <stdint.h>
#include "unyte_udp_queue.h"
#include "unyte_udp_utils.h"
#include "monitoring_worker.h"

/**
 * Input given to the listener thread when creating it.
 */
struct listener_thread_input
{
  unyte_udp_queue_t *output_queue;      /* The queue used to push the segments outside. */
  unyte_udp_queue_t *monitoring_queue;  /* The queue used to push monitoring stats of segments outside. */
  unyte_udp_sock_t *conn;     /* Connection with addr, sockfd */
  uint16_t recvmmsg_vlen;     /* The recvmmsg buffer array size */
  uint nb_parsers;            /* Number of parsers instances to init */
  uint parser_queue_size;     /* Size of parser queue in bytes */
  uint monitoring_delay;      /* Monitoring frequence in seconds */
};

struct parse_worker
{
  unyte_udp_queue_t *queue;
  pthread_t *worker;
  struct parser_thread_input *input;
  struct cleanup_worker *cleanup_worker;
};

struct cleanup_worker
{
  pthread_t *cleanup_thread;
  struct cleanup_thread_input *cleanup_in;
};

struct monitoring_worker
{
  pthread_t *monitoring_thread;
  struct monitoring_thread_input *monitoring_in;
  bool running;
};

/**
 * Threadified app function listening using listener_thread_input struct parameters.
 */
void *t_listener(void *in);

#endif
