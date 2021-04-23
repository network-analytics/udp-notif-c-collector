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

#define USED_VLEN 10
#define MAX_TO_RECEIVE 20

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
  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./client_monitoring <ip> <port>\n");
    exit(1);
  }

  // Initialize collector options
  unyte_options_t options = {0};
  options.address = argv[1];
  options.port = atoi(argv[2]);
  options.recvmmsg_vlen = USED_VLEN;
  options.monitoring_delay = 3;
  options.monitoring_queue_size = 500;

  printf("Listening on %s:%d\n", options.address, options.port);

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector(&options);
  int recv_count = 0;
  int max = MAX_TO_RECEIVE;

  struct read_input input = {0};
  input.output_queue = collector->queue;
  input.stop = 0;

  // Thread simulating collector reading packets
  pthread_t *th_read = (pthread_t *)malloc(sizeof(pthread_t));
  pthread_create(th_read, NULL, t_read, (void *)&input);

  while (recv_count < max)
  {
    void *counter_pointer = unyte_queue_read(collector->monitoring_queue);
    if (counter_pointer == NULL)
    {
      printf("counter_pointer null\n");
      fflush(stdout);
    }
    unyte_sum_counter_t *counter = (unyte_sum_counter_t *)counter_pointer;

    // Getters
    // printf("Thread: %ld\n", get_thread_id(counter));
    // printf("Thread type: %d\n", get_th_type(counter));
    // printf("Generator id: %d\n", get_gen_id(counter));
    // printf("Last msg id: %d\n", get_last_msg_id(counter));
    // printf("Received OK: %d\n", get_ok_seg(counter));
    // printf("Lost: %d\n", get_lost_seg(counter));
    // printf("Reordered: %d\n", get_reordered_seg(counter));

    // print_counters(counter, stdout);
    printf("th_id:%lu|type:%d|ok:%u|lost:%u|reorder:%u|gid:%u|last_mid:%u\n", counter->thread_id, counter->type, counter->segments_count, counter->segments_lost, counter->segments_reordered, counter->generator_id, counter->last_message_id);

    recv_count++;
    fflush(stdout);
    free(counter);
  }

  printf("Shutdown the socket\n");
  close(*collector->sockfd);

  input.stop = 1;
  pthread_join(*th_read, NULL);
  pthread_join(*collector->main_thread, NULL);

  // freeing collector mallocs
  unyte_free_collector(collector);
  free(th_read);
  fflush(stdout);
  return 0;
}
