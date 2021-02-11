#include <pthread.h>
#include "queue.h"
#include "unyte_utils.h"

#ifndef H_UNYTE
#define H_UNYTE

#define DEFAULT_VLEN 10

/**
 * Struct to use to pass information to the user
 */
typedef struct
{
  queue_t *queue;
  pthread_t *main_thread;
  int *sockfd;
} unyte_collector_t;

typedef struct
{
  char *address;
  uint16_t port;
  uint16_t recvmmsg_vlen;
} unyte_options_t;

#define OUTPUT_QUEUE_SIZE 100

unyte_collector_t *unyte_start_collector(unyte_options_t *options);
//TODO: Move signature doc in here
int unyte_free_all(unyte_seg_met_t *seg);
int unyte_free_payload(unyte_seg_met_t *seg);
int unyte_free_header(unyte_seg_met_t *seg);
int unyte_free_metadata(unyte_seg_met_t *seg);

#endif
