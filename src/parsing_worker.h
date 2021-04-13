#ifndef H_PARSING_WORKER
#define H_PARSING_WORKER

#include "queue.h"
#include "segmentation_buffer.h"

struct parser_thread_input
{
  pthread_t parser_thread_id;
  queue_t *input;  /* The feeding queue. */
  queue_t *output; /* The feeded queue */
  struct segment_buffer *segment_buff;
  struct seg_counters *counters;
};

/**
 * Threaded parser function.
 * Calls parser function initializing the parsing worker.
 */
void *t_parser(void *in);

#endif