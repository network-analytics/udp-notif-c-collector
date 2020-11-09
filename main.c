#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include "unyte.h"
#include "unyte_utils.h"
#include "queue.h"

#define PORT 10000

int main()
{
  queue_t *in_msg = start_unyte_collector((uint16_t) PORT);

  while (1)
  {
    /* Read queue */
    struct unyte_segment_with_metadata *seg = (struct unyte_segment_with_metadata *)queue_read(in_msg);

    printHeader(&seg->header, stdout);
    printPayload(seg->payload, seg->header.message_length - seg->header.header_length, stdout);

    free(seg->payload);
    free(seg);
  }

  /* Unreachable... */

  free(in_msg->data);
  free(in_msg);

  return 0;

  /* Not all the memory is freed at this time */
}
