#include "queue.h"

#ifndef H_PARSING_WORKER
#define H_PARSING_WORKER

struct parser_thread_input
{
  queue_t *input;  /* The feeding queue. */
  queue_t *output; /* The feeded queue */
  struct segment_buffer *segment_buff;
};

void *t_parser(void *in);

#endif