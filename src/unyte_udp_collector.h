#ifndef H_UNYTE_UDP_COLLECTOR
#define H_UNYTE_UDP_COLLECTOR

#include <pthread.h>
#include "unyte_udp_queue.h"
#include "unyte_udp_utils.h"

/**
 * Struct to use to pass information to the user
 */
typedef struct
{
  unyte_udp_queue_t *queue;
  unyte_udp_queue_t *monitoring_queue;
  pthread_t *main_thread;
  int *sockfd;
} unyte_udp_collector_t;

typedef struct
{
  int socket_fd; // socket file descriptor
  uint16_t recvmmsg_vlen;
  // parsers
  uint nb_parsers; // number of parsers to instantiate
  // queues sizes
  uint output_queue_size;  // output queue size
  uint parsers_queue_size; // input queue size for every parser
  // monitoring
  uint monitoring_queue_size; // monitoring queue size
  uint monitoring_delay;      // monitoring queue frequence in seconds
} unyte_udp_options_t;

/**
 * Start all the threads of the collector on the given port and return the output segment queue.
 * Messages in the queues are structured in structs unyte_segment_with_metadata like defined in 
 * unyte_udp_utils.h.
 */
unyte_udp_collector_t *unyte_udp_start_collector(unyte_udp_options_t *options);

/**
 * Free all the mem related to the unyte_seg_met_t struct segment
 */
int unyte_udp_free_all(unyte_seg_met_t *seg);

/**
 * Free only the payload
 * pointer still exist but is NULL
 */
int unyte_udp_free_payload(unyte_seg_met_t *seg);

/**
 * Free only the header
 * pointer still exist but is NULL
 */
int unyte_udp_free_header(unyte_seg_met_t *seg);

/**
 * Free only the metadata
 * pointer still exist but is NULL
 */
int unyte_udp_free_metadata(unyte_seg_met_t *seg);

/**
 * Free collector queue data buffer and main thread malloc
 */
int unyte_udp_free_collector(unyte_udp_collector_t *collector);

char *unyte_udp_notif_version();

#endif
