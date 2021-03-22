#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
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
#define LOG_MSG_BETWEEN 50
#define NB_GID 4
#define MSG_TO_RECEIVE 200

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

struct received_msg
{
  // int gid;
  int first_mid;
  int *messages;
  uint total;
};

struct active_gids
{
  int it;
  int *gids;
};

void add_gid(struct active_gids *actives, int gid)
{
  int exists = 0;
  for (int i = 0; i < actives->it; i++)
  {
    if (actives->gids[i] == gid)
    {
      exists = 1;
      break;
    }
  }
  if (exists == 0)
  {
    actives->gids[actives->it] = gid;
    actives->it++;
  }
}

int active_gids_full(struct active_gids *actives, int *full_index)
{
  for (int i = 0; i < actives->it; i++)
  {
    if (full_index[actives->gids[i]] == 0)
    {
      return 0;
    }
  }
  return 1;
}

struct received_msg *init_stats(uint nb_gid, uint nb_messages)
{
  struct received_msg *messages = (struct received_msg *)malloc(sizeof(struct received_msg) * nb_gid);
  struct received_msg *cur = messages;

  for (uint i = 0; i < nb_gid; i++)
  {
    // cur->gid = -1;
    cur->first_mid = -1;
    cur->messages = (int *)malloc(sizeof(int) * (nb_messages + nb_gid));
    memset(cur->messages, -1, sizeof(int) * (nb_messages + nb_gid));
    cur->total = nb_messages;
    cur++;
  }
  return messages;
}

void update_stats(struct received_msg *messages, uint index, uint mid, uint gid, int *full_index)
{
  struct received_msg *message_stats = messages + index;
  if (message_stats->first_mid < 0)
  {
    message_stats->first_mid = mid;
  }
  if (message_stats->total > (mid - message_stats->first_mid))
  {
    // printf("%d|%d\n", message_stats->total, mid - message_stats->first_mid);
    message_stats->messages[mid - message_stats->first_mid] = 1;
  }
  else
  {
    full_index[gid] = 1;
  }
}

void clean_received_msg(struct received_msg *messages, uint nb_gid)
{
  for (uint i = 0; i < nb_gid; i++)
  {
    free((messages + i)->messages);
  }
  free(messages);
}

uint get_gid_index(int *indexes, uint gid, uint *cur_new)
{
  if (indexes[gid] < 0)
  {
    indexes[gid] = *cur_new;
    *cur_new += 1;
  }
  return indexes[gid];
}

uint get_lost_messages(struct received_msg *messages, uint index)
{
  struct received_msg *message_stats = messages + index;
  uint lost = 0;
  for (uint i = 0; i < message_stats->total; i++)
  {
    if (message_stats->messages[i] < 0)
    {
      lost++;
    }
  }
  return lost;
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

  uint number_gids = NB_GID;
  uint messages_to_recv = MSG_TO_RECEIVE;

  uint cur_new = 0;
  struct received_msg *message_stats = init_stats(number_gids, messages_to_recv);
  uint max_gid = 1000;
  int *indexes = (int *)malloc(sizeof(int) * max_gid);
  int *full_index = (int *)malloc(sizeof(int) * max_gid);
  struct active_gids *actives = (struct active_gids *)malloc(sizeof(struct active_gids));
  actives->it = 0;
  actives->gids = (int *)malloc(sizeof(int) * max_gid);

  memset(indexes, -1, sizeof(int) * max_gid);
  memset(full_index, 0, sizeof(int) * max_gid);

  uint counter = 0;
  int first = 1;

  struct timespec start;
  struct timespec stop;
  struct timespec diff;
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

    uint gid_index = get_gid_index(indexes, seg->header->generator_id, &cur_new);
    update_stats(message_stats, gid_index, seg->header->message_id, seg->header->generator_id, full_index);

    add_gid(actives, seg->header->generator_id);

    if ((counter % LOG_MSG_BETWEEN) == 0)
    {
      if (first)
      {
        first = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
      }
      else
      {
        clock_gettime(CLOCK_MONOTONIC, &stop);
        time_diff(&diff, &stop, &start, counter, 0);
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (active_gids_full(actives, full_index))
        {
          fflush(stdout);
          unyte_free_all(seg);
          break;
        }
      }
    }
    counter++;
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

  for (uint i = 0; i < max_gid; i++)
  {
    if (indexes[i] < 0)
      continue;

    uint lost = get_lost_messages(message_stats, indexes[i]);
    printf("Lost;%d;%d;%d\n", i, lost, (message_stats + indexes[i])->total);
  }

  /* Free last packets in the queue */
  while (is_queue_empty(collector->queue) != 0)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    unyte_free_all(seg);
  }

  unyte_free_collector(collector);
  clean_received_msg(message_stats, number_gids);
  free(indexes);
  free(full_index);
  free(actives->gids);
  free(actives);
  return 0;
}
