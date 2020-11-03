#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "queue.h"
#include "unyte_utils.h"

int parser(queue_t *q)
{
  while (1)
  {
    char *segment = (char *)queue_read(q);
    /* Process segment */
    if (strcmp(segment, "exit") == 0)
    {
      printf("Exit parsing thread\n");
      fflush(stdout);
      return 0;
    }
    struct unyte_segment *parsed_segment = parse(segment);
    printHeader(&parsed_segment->header, stdout);
    printPayload(parsed_segment->payload, parsed_segment->header.message_length - parsed_segment->header.header_length, stdout);
  }
  return 0;
}

void *t_parser(void *q)
{
  parser((queue_t *)q);
  pthread_exit(NULL);
}