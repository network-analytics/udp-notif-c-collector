#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../src/hexdump.h"
#include "../src/unyte_udp_collector.h"
#include "../src/monitoring_worker.h"

#define USED_VLEN 10
#define MAX_TO_RECEIVE 20

struct read_input
{
  unyte_udp_queue_t *output_queue;
  int stop;
};

void *t_read(void *input)
{
  struct read_input *in = (struct read_input *)input;
  unyte_udp_queue_t *output_queue = in->output_queue;
  while (in->stop == 0)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_udp_queue_read(output_queue);
    unyte_udp_free_all(seg);
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

  // Create a udp socket with default socket buffer
  int socketfd = unyte_udp_create_socket(argv[1], argv[2], DEFAULT_SK_BUFF_SIZE);
  printf("Listening on %s:%s\n", argv[1], argv[2]);

  // Initialize collector options
  unyte_udp_options_t options = {0};
  options.recvmmsg_vlen = USED_VLEN;
  options.socket_fd = socketfd; // passing socket file descriptor to listen to
  options.monitoring_delay = 2;
  options.monitoring_queue_size = 500;

  // Initialize collector
  unyte_udp_collector_t *collector = unyte_udp_start_collector(&options);
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
    void *counter_pointer = unyte_udp_queue_read(collector->monitoring_queue);
    if (counter_pointer == NULL)
    {
      printf("counter_pointer null\n");
      fflush(stdout);
    }
    unyte_udp_sum_counter_t *counter = (unyte_udp_sum_counter_t *)counter_pointer;

    // Getters
    // printf("Thread id: %ld\n", unyte_udp_get_thread_id(counter));
    // printf("Thread type: %d\n", unyte_udp_get_th_type(counter));
    // printf("Observation domain id: %d\n", unyte_udp_get_od_id(counter));
    // printf("Last msg id: %d\n", unyte_udp_get_last_msg_id(counter));
    // printf("Received OK: %d\n", unyte_udp_get_received_seg(counter));
    // printf("Dropped: %d\n", unyte_udp_get_dropped_seg(counter));
    // printf("Reordered: %d\n", unyte_udp_get_reordered_seg(counter));

    unyte_udp_print_counters(counter, stdout);

    // listening worker is losing segments
    if (counter->type == LISTENER_WORKER && unyte_udp_get_dropped_seg(counter) > 0)
    {
      // The input_queue used by the parser is becoming full and is dropping messages. Try instantiating more
      // parsers or increasing the parsers_queue_size.
      // printf("/!\\ Losing messages on input_queue. The parsers threads are not consuming the segments that fast.\n");
    }
    // parser worker is losing segments
    else if (counter->type == PARSER_WORKER && unyte_udp_get_dropped_seg(counter) > 0)
    {
      // The output_queue used by the client is becoming full and is dropping messages. Try multithreading the
      // client (this main thread) to consume faster or increasing the output_queue_size.
      // printf("/!\\ Losing messages on output_queue. The main client is not consuming the parsed messages that fast.\n");
    }

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
  unyte_udp_free_collector(collector);
  free(th_read);
  fflush(stdout);
  return 0;
}
