#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "../src/hexdump.h"
#include "../src/unyte_collector.h"
#include "../src/unyte_utils.h"
#include "../src/queue.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define USED_VLEN 10
#define PARSERS_NB 10
#define OUTPUT_QUEUE 1000
#define PARSER_QUEUE 500
// SK_BUFF = 20MB
#define SK_BUFF 20971520
#define MAX_GEN_ID 1000
#define MSG_ID_MAX 4294967295
#define LOG_MSG_BETWEEN 200

void time_diff(struct timespec *diff, struct timespec *stop, struct timespec *start, int messages, pthread_t thread_id)
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
  printf("%ld;%d;%ld,%06ld\n", thread_id, messages, diff->tv_sec * 1000 + diff->tv_nsec / 1000000, diff->tv_nsec % 1000000);
}

struct message_stat
{
  uint last_msg_id;
  uint packet_count;
  uint packet_lost;
  uint packet_reorder;
};

void update_message_stats(struct message_stat *message_stat, unyte_seg_met_t *seg)
{
  if (message_stat->last_msg_id == 0)
  {
    message_stat->last_msg_id = seg->header->message_id;
    message_stat->packet_count += 1;
  }
  else if ((seg->header->message_id - message_stat->last_msg_id) == 1) // received 1 packet
  {
    message_stat->packet_count += 1;
    message_stat->last_msg_id = seg->header->message_id;
  }
  else if ((seg->header->message_id - message_stat->last_msg_id) > 1) // packet lost
  {
    message_stat->packet_count += 1;
    message_stat->packet_lost += seg->header->message_id - message_stat->last_msg_id - 1;
    message_stat->last_msg_id = seg->header->message_id;
  }
  else // packet reorder
  {
    message_stat->packet_count += 1;
    message_stat->packet_reorder += 1;
  }
}

void restart_message_stats(struct message_stat *message_stat)
{
  message_stat->packet_count = 0;
  message_stat->packet_lost = 0;
  message_stat->packet_reorder = 0;
}

void print_message_stats(uint gid, struct message_stat *message_stat)
{
  printf("%d;%d;%d;%d\n", gid, message_stat->packet_lost, message_stat->packet_reorder, message_stat->packet_count);
}

int main(int argc, char *argv[])
{
  // Initialize collector options
  unyte_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.recvmmsg_vlen = USED_VLEN;
  options.nb_parsers = PARSERS_NB;
  options.output_queue_size = OUTPUT_QUEUE;
  options.parsers_queue_size = PARSER_QUEUE;
  options.socket_buff_size = SK_BUFF;

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector(&options);

  struct message_stat message_stats[MAX_GEN_ID];

  while (1)
  {
    /* Read queue */
    void *seg_pointer = unyte_queue_read(collector->queue);
    if (seg_pointer == NULL)
    {
      printf("seg_pointer null\n");
      fflush(stdout);
    }
    unyte_seg_met_t *seg = (unyte_seg_met_t *)seg_pointer;
    struct message_stat *current_stat = &message_stats[seg->header->generator_id];
    update_message_stats(current_stat, seg);

    if ((current_stat->packet_count % LOG_MSG_BETWEEN) == 0)
    {
      print_message_stats(seg->header->generator_id, current_stat);
      restart_message_stats(current_stat);
    }

    // printHeader(seg->header, stdout);
    // printf("REc:%d\n", seg->header->message_id);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
    // printf("counter : %d\n", recv_count);
    fflush(stdout);

    /* Struct frees */
    unyte_free_all(seg);
  }
  return 0;
}
