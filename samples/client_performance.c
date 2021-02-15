#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "../src/lib/hexdump.h"
#include "../src/unyte.h"
#include "../src/unyte_utils.h"
#include "../src/queue.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define MAX_TO_RECEIVE 10000
#define USED_VLEN 10
#define TIME_BETWEEN 200

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

void time_diff(struct timespec *diff, struct timespec *stop, struct timespec *start, int messages, int lost_messages)
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
  printf("%d;%d;%ld,%06ld\n", messages, lost_messages, diff->tv_sec * 1000 + diff->tv_nsec / 1000000, diff->tv_nsec % 1000000);
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
  int lost_packets = 0;

  struct timespec start;
  struct timespec stop;
  struct timespec diff;

  struct message_obs_id *msg_obs_ids = (struct message_obs_id *)malloc(msg_max->max_to_receive * sizeof(struct message_obs_id));
  memset(msg_obs_ids, 0, msg_max->max_to_receive * sizeof(struct message_obs_id));

  for (int i = 0; i < msg_max->max_to_receive; i++)
  {
    msg_obs_ids[i].observation_id = -1;
  }

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
        time_diff(&diff, &stop, &start, recv_count, lost_packets);
        clock_gettime(CLOCK_MONOTONIC, &start);
        lost_packets = 0;
      }
    }
    recv_count++;

    if (msg_obs_ids[seg->header->generator_id].observation_id < 0)
    {
      msg_obs_ids[seg->header->generator_id].segment_id = seg->header->f_num;
      msg_obs_ids[seg->header->generator_id].lost = 0;
      msg_obs_ids[seg->header->generator_id].observation_id = seg->header->generator_id;
      // printf("new observation id: %d\n", seg->header->generator_id);
    }
    else
    {
      if (seg->header->f_num - msg_obs_ids[seg->header->generator_id].segment_id > 1)
      {
        msg_obs_ids[seg->header->generator_id].lost += (seg->header->f_num - msg_obs_ids[seg->header->generator_id].segment_id) - 1;
        lost_packets += (seg->header->f_num - msg_obs_ids[seg->header->generator_id].segment_id) - 1;
        printf("-> Lost %d messages\n", (seg->header->f_num - msg_obs_ids[seg->header->generator_id].segment_id) - 1);
      }
      // printf("--> new id %d\n", seg->header->f_num);
      msg_obs_ids[seg->header->generator_id].segment_id = seg->header->f_num;
    }
    // printHeader(seg->header, stdout);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
    // printf("counter : %d\n", recv_count);
    fflush(stdout);

    /* Struct frees */
    unyte_free_all(seg);
  }

  // Get last time
  clock_gettime(CLOCK_MONOTONIC, &stop);
  time_diff(&diff, &stop, &start, recv_count, lost_packets);
  clock_gettime(CLOCK_MONOTONIC, &start);

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

  // freeing collector mallocs
  unyte_free_collector(collector);
  
  free(msg_obs_ids);
  free(msg_max);
  return 0;
}
