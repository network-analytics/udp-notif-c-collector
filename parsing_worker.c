#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "queue.h"
#include "unyte_utils.h"

int parser(queue_t *q)
{
  while (1)
  {
    /* char *segment = (char *)queue_read(q); */
    struct unyte_minimal *queue_data = (struct unyte_minimal *)queue_read(q);

    /* Process segment */
    if (strcmp(queue_data->buffer, "exit") == 0)
    {
      printf("Exit parsing thread\n");
      fflush(stdout);
      return 0;
    }
    struct unyte_segment *parsed_segment = parse(queue_data->buffer);
    
    printHeader(&parsed_segment->header, stdout);
    printPayload(parsed_segment->payload, parsed_segment->header.message_length - parsed_segment->header.header_length, stdout);
    
    free(parsed_segment->payload);
    free(&parsed_segment->header);
    free(queue_data->buffer);
    free(queue_data);
  }
  return 0;
}

void *t_parser(void *q)
{
  parser((queue_t *)q);
  pthread_exit(NULL);
}
