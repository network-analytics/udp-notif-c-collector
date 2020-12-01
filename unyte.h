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
} collector_t;

#define OUTPUT_QUEUE_SIZE 100
#define PARSER_NUMBER 10

collector_t *start_unyte_collector(uint16_t port, uint32_t addr);
int unyte_free_all(struct unyte_segment_with_metadata *seg);
int unyte_free_payload(struct unyte_segment_with_metadata *seg);
int unyte_free_header(struct unyte_segment_with_metadata *seg);
int unyte_free_metadata(struct unyte_segment_with_metadata *seg);

#endif
