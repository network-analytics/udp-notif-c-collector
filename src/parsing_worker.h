#ifndef H_PARSING_WORKER
#define H_PARSING_WORKER

#include "unyte_udp_queue.h"
#include "segmentation_buffer.h"
#include "monitoring_worker.h"

struct parser_thread_input
{
  pthread_t parser_thread_id;
  unyte_udp_queue_t *input;  /* The feeding queue. */
  unyte_udp_queue_t *output; /* The feeded queue */
  struct segment_buffer *segment_buff;
  unyte_seg_counters_t *counters;
  int monitoring_running;
  bool legacy_proto;
};

/**
 * Threaded parser function.
 * Calls parser function initializing the parsing worker.
 */
void *t_parser(void *in);

#endif