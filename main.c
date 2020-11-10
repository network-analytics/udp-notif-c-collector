#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include "unyte.h"
#include "unyte_utils.h"
#include "queue.h"

#define PORT 9341

int main()
{
  queue_t *in_msg = start_unyte_collector((uint16_t) PORT);
  int recv_count = 0;

  while (1)
  {
    /* Read queue */
    struct unyte_segment_with_metadata *seg = (struct unyte_segment_with_metadata *)queue_read(in_msg);

    recv_count++;
    printHeader(&seg->header, stdout);
    printf("counter : %d", recv_count);
    fflush(stdout);

    free(seg->payload);
    free(seg);
  }

  /* Unreachable... */

  free(in_msg->data);
  free(in_msg);

  return 0;

  /* Not all the memory is freed at this time */
}
