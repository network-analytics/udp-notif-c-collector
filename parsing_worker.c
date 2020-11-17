#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "queue.h"
#include "unyte_utils.h"
#include "parsing_worker.h"

/**
 * Parser that receive unyte_minimal structs stream from the Q queue.
 * It transforms it into unyte_segment_with_metadata structs and push theses to the OUTPUT queue.
 */
int parser(struct parser_thread_input *in)
{
  while (1)
  {
    /* char *segment = (char *)queue_read(q); */
    struct unyte_minimal *queue_data = (struct unyte_minimal *)queue_read(in->input);

    /* Process segment */
    if (strcmp(queue_data->buffer, "exit") == 0)
    {
      printf("Exit parsing thread\n");
      fflush(stdout);
      return 0;
    }

    /* Can do better */
    struct unyte_segment_with_metadata *parsed_segment = parse_with_metadata(queue_data->buffer, queue_data);

    /* Not useful anymore */
    free(queue_data->buffer);
    free(queue_data);

    /* Check about fragmentation */

    if (parsed_segment->header->header_length <= 12)
    {
      printf("parser pushed \n");
      printf("parsed_segment : %p\n", parsed_segment);
      printf("on queue : %p\n", in->input);
      queue_write(in->output, parsed_segment);
      printf("done.\n");
    }
    else
    {
      printf("segmented, not pushed");
      fflush(stdout);
      /* Discarding the segment while fragmentation is not fully implemented.
         Must add frees 
      */
    }
  }
  return 0;
}

/**
 * Threaded parser function
 */
void *t_parser(void *in)
{
  parser((struct parser_thread_input *)in);
  free(in);
  pthread_exit(NULL);
}
