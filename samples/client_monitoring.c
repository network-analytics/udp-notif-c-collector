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
#define MAX_TO_RECEIVE 100

struct read_input
{
  queue_t *output_queue;
  int stop;
};

void *t_read(void *input)
{
  struct read_input *in = (struct read_input *)input;
  queue_t *output_queue = in->output_queue;
  while (in->stop == 0)
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
  options.monitoring_delay = 2;
  options.monitoring_queue_size = 500;

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector(&options);
  int recv_count = 0;
  int max = MAX_TO_RECEIVE;

  struct read_input input = {0};
  input.output_queue = collector->queue;
  input.stop = 0;
  pthread_t *th_read = (pthread_t *)malloc(sizeof(pthread_t));
  pthread_create(th_read, NULL, t_read, (void *)&input);

  while (recv_count < max)
  {
    // sleep(10);
    // break;
    /* Read queue */
    void *counter_pointer = unyte_queue_read(collector->monitoring_queue);
    if (counter_pointer == NULL)
    {
      printf("counter_pointer null\n");
      fflush(stdout);
    }
    unyte_seg_counters_t *counters = (unyte_seg_counters_t *)counter_pointer;
    // print_counters(counters, stdout);
    printf("%lu|type:%d|%u|%u|%u|%u|%u\n", counters->thread_id, counters->type, counters->segments_count, counters->segments_lost, counters->segments_reordered, counters->last_generator_id, counters->last_message_id);
    recv_count++;
    fflush(stdout);
    free(counters);
  }

  printf("Shutdown the socket\n");
  close(*collector->sockfd);

  input.stop = 1;
  pthread_join(*th_read, NULL);
  pthread_join(*collector->main_thread, NULL);

  free(th_read);
  // freeing collector mallocs
  unyte_free_collector(collector);
  fflush(stdout);
  return 0;
}
