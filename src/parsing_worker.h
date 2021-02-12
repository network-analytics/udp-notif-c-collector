#include "queue.h"

#ifndef H_PARSING_WORKER
#define H_PARSING_WORKER

struct parser_thread_input
{
  pthread_t parser_thread_id;
  queue_t *input;  /* The feeding queue. */
  queue_t *output; /* The feeded queue */
  struct segment_buffer *segment_buff;
};

void *t_parser(void *in);
unyte_seg_met_t *create_assembled_msg(char *complete_msg, unyte_seg_met_t *src_parsed_segment, uint16_t total_payload_byte_size);

#endif