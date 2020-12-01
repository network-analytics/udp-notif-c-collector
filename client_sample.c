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
#define ADDR 0

int main()
{

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector((uint16_t)PORT, (uint32_t)ADDR);
  int recv_count = 0;
  int max = 10;

  while (recv_count < max)
  {
    /* Read queue */
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);

    /* Processing sample */
    recv_count++;
    /* printHeader(seg->header, stdout); */
    /* hexdump(seg->payload, seg->header->message_length - seg->header->header_length);*/
    /* printf("counter : %d\n", recv_count); */
    fflush(stdout);

    /* Struct frees */
    unyte_free_all(seg);
  }

  printf("Shutdown the socket\n");
  shutdown(*collector->sockfd, SHUT_RDWR);
  close(*collector->sockfd);
  pthread_join(*collector->main_thread, NULL);

  /* Free last packets in the queue */
  while (is_queue_empty(collector->queue) != 0)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    unyte_free_all(seg);
  }

  free(collector->queue->data);
  free(collector->queue);
  free(collector->main_thread);
  free(collector);

  return 0;
}
