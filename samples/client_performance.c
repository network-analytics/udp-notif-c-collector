#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "../src/lib/hexdump.h"
#include "../src/unyte_collector.h"
#include "../src/unyte_utils.h"
#include "../src/queue.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define MAX_TO_RECEIVE 10000
#define USED_VLEN 10
#define TIME_BETWEEN 1000
#define LAST_GEN_ID 49993648

struct message_obs_id
{
  int observation_id;
  int segment_id;
  int lost;
};

struct messages_max_log {
  int max_to_receive;
  int time_between_log;
};

void time_diff(struct timespec *diff, struct timespec *stop, struct timespec *start, int messages)
{
  if (stop->tv_nsec < start->tv_nsec)
  {
    /* here we assume (stop->tv_sec - start->tv_sec) is not zero */
    diff->tv_sec = stop->tv_sec - start->tv_sec - 1;
    diff->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
  }
  else
  {
    diff->tv_sec = stop->tv_sec - start->tv_sec;
    diff->tv_nsec = stop->tv_nsec - start->tv_nsec;
  }
  printf("%d;%ld,%06ld\n", messages, diff->tv_sec * 1000 + diff->tv_nsec / 1000000, diff->tv_nsec % 1000000);
}

void parse_arguments(unyte_options_t *options, struct messages_max_log *msg_max_log, int argc, char *argv[]) {
  options->address = ADDR;
  options->port = PORT;
  options->recvmmsg_vlen = USED_VLEN;
  msg_max_log->max_to_receive = MAX_TO_RECEIVE;
  msg_max_log->time_between_log = TIME_BETWEEN;

  if (argc == 6) {
    // Usage: ./client_perfomance <max_to_receive> <time_between_log> <vlen> <src_IP> <src_port>
    msg_max_log->max_to_receive = atoi(argv[1]);
    msg_max_log->time_between_log = atoi(argv[2]);
    options->recvmmsg_vlen = atoi(argv[3]);
    options->address = argv[4];
    options->port = atoi(argv[5]);

    printf("PARAMS: %d|%d|%d|%s|%d\n", msg_max_log->max_to_receive, msg_max_log->time_between_log, options->recvmmsg_vlen, options->address, options->port);
  } else {
    printf("Using default values\n");
  }
}

int main(int argc, char *argv[])
{
  // Initialize collector options
  unyte_options_t options = {0};

  struct messages_max_log *msg_max = (struct messages_max_log *)malloc(sizeof(struct messages_max_log));

  parse_arguments(&options, msg_max, argc, argv);
  // printf("Listening on %s:%d\n", options.address, options.port);

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector(&options);
  int recv_count = 0;
  int first = 1;

  struct timespec start;
  struct timespec stop;
  struct timespec diff;

  int received_gids[msg_max->max_to_receive];
  memset(received_gids, -1, msg_max->max_to_receive * sizeof(int));

  while (recv_count < msg_max->max_to_receive)
  {
    /* Read queue */
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    
    if (recv_count % msg_max->time_between_log == 0)
    {
      if (first)
      {
        first = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
      }
      else
      {
        clock_gettime(CLOCK_MONOTONIC, &stop);
        time_diff(&diff, &stop, &start, recv_count);
        clock_gettime(CLOCK_MONOTONIC, &start);
      }
    }
    recv_count++;

    // break when receive last_gen_id from scapy
    if (seg->header->generator_id == LAST_GEN_ID) {
      fflush(stdout);
      unyte_free_all(seg);
      break;
    }

    received_gids[seg->header->generator_id] = 1;

    // printHeader(seg->header, stdout);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
    // printf("counter : %d\n", recv_count);
    fflush(stdout);

    /* Struct frees */
    unyte_free_all(seg);
  }

  // Get last time
  clock_gettime(CLOCK_MONOTONIC, &stop);
  time_diff(&diff, &stop, &start, recv_count);
  clock_gettime(CLOCK_MONOTONIC, &start);

  printf("Shutdown the socket\n");
  shutdown(*collector->sockfd, SHUT_RDWR); //TODO: à valider/force empty queue (?)
  close(*collector->sockfd);
  pthread_join(*collector->main_thread, NULL);

  /* Free last packets in the queue */
  while (is_queue_empty(collector->queue) != 0)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    if (seg->header->generator_id != LAST_GEN_ID) {
      received_gids[seg->header->generator_id] = 1;
    }
    unyte_free_all(seg);
  }

  int lost_messages = 0;
  for (int i = 0; i < msg_max->max_to_receive; i++)
  {
    if (received_gids[i] == -1) {
      lost_messages += 1;
    }
  }
  printf("Lost;%d/%d;%f\n", lost_messages, msg_max->max_to_receive,((double)lost_messages/(double)msg_max->max_to_receive) * 100);

  // freeing collector mallocs
  unyte_free_collector(collector);
  fflush(stdout);
  
  free(msg_max);
  return 0;
}
