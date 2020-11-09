#include "queue.h"

#ifndef H_PARSING_WORKER
#define H_PARSING_WORKER

struct parser_thread_input
{
  queue_t *input;  /* The feeding queue. */
  queue_t *output; /* The feeded queue */
};

void *t_parser(void *in);

#endif