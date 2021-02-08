#include <pthread.h>
#include "queue.h"
#include "unyte_utils.h"

#ifndef H_UNYTE
#define H_UNYTE

/**
 * Struct to use to pass information to the user
 */
typedef struct
{
  queue_t *queue;
  pthread_t *main_thread;
  int *sockfd;
} unyte_collector_t;

#define OUTPUT_QUEUE_SIZE 100
#define PARSER_NUMBER 10

unyte_collector_t *unyte_start_collector(char *addr, uint16_t port);
int unyte_free_all(unyte_seg_met_t *seg);
int unyte_free_payload(unyte_seg_met_t *seg);
int unyte_free_header(unyte_seg_met_t *seg);
int unyte_free_metadata(unyte_seg_met_t *seg);

#endif
