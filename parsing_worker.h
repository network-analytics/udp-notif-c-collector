#include "queue.h"

#ifndef H_PARSING_WORKER
#define H_PARSING_WORKER

struct parser_thread_input
{
  pthread_t thread_id;
  queue_t *input;  /* The feeding queue. */
  queue_t *output; /* The feeded queue */
  struct segment_buffer *segment_buff;

  //TODO: remove and launch both thread on listening_worker
  pthread_t *clean_up_thread;
  struct segment_cleanup *seg_cleanup_in;
};

void *t_parser(void *in);

#endif