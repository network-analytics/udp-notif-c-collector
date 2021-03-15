#ifndef H_UNYTE_COLLECTOR
#define H_UNYTE_COLLECTOR

#include <pthread.h>
#include "queue.h"
#include "unyte_utils.h"

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
  // global
  char *address;
  uint16_t port;
  // listener
  uint16_t recvmmsg_vlen;
  // parsers
  uint nb_parsers;          // number of parsers to instantiate
  // queues sizes
  uint output_queue_size;   // output queue size in bytes
  uint parsers_queue_size;  // input queue size in bytes for every parser
} unyte_options_t;

#define OUTPUT_QUEUE_SIZE 1000

/**
 * Start all the subprocesses of the collector on the given port and return the output segment queue.

 * Messages in the queues are structured in structs unyte_segment_with_metadata like defined in 
 * unyte_utils.h.
 */
unyte_collector_t *unyte_start_collector(unyte_options_t *options);

/**
 * Free all the mem related to the unyte_seg_met_t struct segment
 */
int unyte_free_all(unyte_seg_met_t *seg);

/**
 * Free only the payload
 * pointer still exist but is NULL
 */
int unyte_free_payload(unyte_seg_met_t *seg);

/**
 * Free only the header
 * pointer still exist but is NULL
 */
int unyte_free_header(unyte_seg_met_t *seg);

/**
 * Free only the metadata
 * pointer still exist but is NULL
 */
int unyte_free_metadata(unyte_seg_met_t *seg);

/**
 * Free collector queue data buffer and main thread malloc
 */
int unyte_free_collector(unyte_collector_t *collector);

#endif
