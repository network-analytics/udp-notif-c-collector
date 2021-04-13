#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "../src/hexdump.h"
#include "../src/unyte_collector.h"
#include "../src/unyte_utils.h"
#include "../src/queue.h"
#include "../src/monitoring_worker.h"

#define PORT 10000
#define ADDR "192.168.122.1"
#define USED_VLEN 10
#define MAX_TO_RECEIVE 5

void *t_read(void *input)
{
  queue_t *output_queue = (queue_t *)input;
  while (1)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(output_queue);
    unyte_free_all(seg);
  }
  return NULL;
}

int main()
{
  printf("Listening on %s:%d\n", ADDR, PORT);
  // Initialize collector options
  unyte_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.recvmmsg_vlen = USED_VLEN;
  options.monitoring_delay = 1;

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector(&options);
  int recv_count = 0;
  int max = MAX_TO_RECEIVE;

  pthread_t *th_read = (pthread_t *)malloc(sizeof(pthread_t));
  pthread_create(th_read, NULL, t_read, (void *)collector->queue);

  while (recv_count < max)
  {
    /* Read queue */
    void *counter_pointer = unyte_queue_read(collector->monitoring_queue);
    if (counter_pointer == NULL)
    {
      printf("counter_pointer null\n");
      fflush(stdout);
    }
    unyte_seg_counters_t *counters = (unyte_seg_counters_t *)counter_pointer;
    printf("*** Counters ***\n");
    printf("OK: %u\n", counters->segments_count);
    printf("Lost: %u\n", counters->segments_lost);
    printf("Reorder: %u\n", counters->segments_reordered);
    printf("Last gen id: %u\n", counters->last_generator_id);
    printf("Last msg id: %u\n", counters->last_message_id);
    printf("***** End ******\n");
    recv_count++;
    fflush(stdout);
    free(counters);
  }

  printf("Shutdown the socket\n");
  close(*collector->sockfd);

  pthread_cancel(*th_read);
  pthread_join(*th_read, NULL);
  pthread_join(*collector->main_thread, NULL);

  /* Free last packets in the queue */
  while (is_queue_empty(collector->queue) != 0)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    unyte_free_all(seg);
  }

  while(is_queue_empty(collector->monitoring_queue) != 0)
  {
    void *counter_pointer = unyte_queue_read(collector->monitoring_queue);
    free(counter_pointer);
  }

  free(th_read);
  // freeing collector mallocs
  unyte_free_collector(collector);
  fflush(stdout);
  return 0;
}
