#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include "unistd.h"
#include "hexdump.h"
#include "unyte.h"
#include "unyte_utils.h"
#include "queue.h"

#define PORT 9341

int main()
{

  /* Initialize collector */
  collector_t *collector = start_unyte_collector((uint16_t) PORT);
  int recv_count = 0;

  while (0)
  {
    /* Read queue */
    struct unyte_segment_with_metadata *seg = (struct unyte_segment_with_metadata *)queue_read(collector->queue);

    /* Processing sample */
    recv_count++;
    printHeader(seg->header, stdout);
    hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
    printf("counter : %d", recv_count);
    fflush(stdout);


    /* Struct frees */
    unyte_free_all(seg);
  }

  /* Unreachable until interruption handling */

  printf("Shutdown the socket\n");
  shutdown(*collector->sockfd, SHUT_RDWR);
  close(*collector->sockfd);
  pthread_join(*collector->main_thread, NULL);

  free(collector->queue);
  free(collector);

  return 0;
}
