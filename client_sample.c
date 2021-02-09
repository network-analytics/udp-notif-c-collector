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

#define PORT 8081
#define ADDR "192.168.0.17"
#define USED_VLEN 5
#define MAX_TO_RECEIVE 10

int main()
{
  printf("Listening on %s:%d\n", ADDR, PORT);
  // Initialize collector options
  unyte_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.recvmmsg_vlen = USED_VLEN;

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector(&options);
  int recv_count = 0;
  int max = MAX_TO_RECEIVE;

  while (recv_count < max)
  {
    /* Read queue */
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);

    /* Processing sample */
    recv_count++;
    // printHeader(seg->header, stdout);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
    // printf("counter : %d\n", recv_count);
    fflush(stdout);

    /* Struct frees */
    unyte_free_all(seg);
  }

  printf("Shutdown the socket\n");
  shutdown(*collector->sockfd, SHUT_RDWR); //TODO: à valider/force empty queue (?)
  close(*collector->sockfd);
  pthread_join(*collector->main_thread, NULL);

  /* Free last packets in the queue */
  while (is_queue_empty(collector->queue) != 0)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    unyte_free_all(seg);
  }

    //TODO Should we encapsulate this in a function?
  free(collector->queue->data);
  free(collector->queue);
  free(collector->main_thread);
  free(collector);

  return 0;
}
