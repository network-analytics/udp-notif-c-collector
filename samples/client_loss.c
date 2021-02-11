#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../src/lib/hexdump.h"
#include "../src/unyte.h"
#include "../src/unyte_utils.h"
#include "../src/queue.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define MAX_TO_RECEIVE 90000
#define USED_VLEN 10

struct message_obs_id
{
  int observation_id;
  int segment_id;
  int lost;
};

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

  struct message_obs_id *msg_obs_ids = (struct message_obs_id *)malloc(MAX_TO_RECEIVE * sizeof(struct message_obs_id));
  memset(msg_obs_ids, 0, MAX_TO_RECEIVE * sizeof(struct message_obs_id));

  for (int i = 0; i < MAX_TO_RECEIVE; i++)
  {
    msg_obs_ids[i].observation_id = -1;
  }

  while (recv_count < max)
  {
    /* Read queue */
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);

    /* Processing sample */
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
        printf("-> Lost %d messages\n", (seg->header->f_num - msg_obs_ids[seg->header->generator_id].segment_id) - 1);
      }
      // printf("--> new id %d\n", seg->header->f_num);
      msg_obs_ids[seg->header->generator_id].segment_id = seg->header->f_num;
    }
    // printHeader(seg->header, stdout);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
    /* printf("counter : %d\n", recv_count); */
    fflush(stdout);

    /* Struct frees */
    unyte_free_all(seg);
  }

  for (int i = 0; i < MAX_TO_RECEIVE; i++)
  {
    if (msg_obs_ids[i].lost > 0)
    {
      printf("Lost %d messages\n", msg_obs_ids[i].lost);
    }
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

  free(collector->queue->data);
  free(collector->queue);
  free(collector->main_thread);
  free(collector);
  free(msg_obs_ids);

  return 0;
}
