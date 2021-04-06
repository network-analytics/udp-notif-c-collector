#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "../src/hexdump.h"
#include "../src/unyte_collector.h"
#include "../src/unyte_utils.h"
#include "../src/queue.h"

#define PORT 10000
#define ADDR "192.168.0.17"
#define MAX_TO_RECEIVE 1000
#define USED_VLEN 50
#define TIME_BETWEEN 1000
#define LAST_GEN_ID 49993648
#define NB_THREADS 4

struct messages_max_log
{
  int max_to_receive;
  int time_between_log;
};

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

void parse_arguments(unyte_options_t *options, struct messages_max_log *msg_max_log, int argc, char *argv[])
{
  options->address = ADDR;
  options->port = PORT;
  options->recvmmsg_vlen = USED_VLEN;
  options->output_queue_size = 0;
  options->nb_parsers = 0;
  options->socket_buff_size = 0; //use default 20MB
  options->parsers_queue_size = 0;
  msg_max_log->max_to_receive = MAX_TO_RECEIVE;
  msg_max_log->time_between_log = TIME_BETWEEN;

  if (argc == 6)
  {
    // Usage: ./client_perfomance <max_to_receive> <time_between_log> <vlen> <src_IP> <src_port>
    msg_max_log->max_to_receive = atoi(argv[1]);
    msg_max_log->time_between_log = atoi(argv[2]);
    options->recvmmsg_vlen = atoi(argv[3]);
    options->address = argv[4];
    options->port = atoi(argv[5]);

    printf("PARAMS: %d|%d|%d|%s|%d\n", msg_max_log->max_to_receive, msg_max_log->time_between_log, options->recvmmsg_vlen, options->address, options->port);
  } else if (argc == 10) {
    // Usage: ./client_perfomance <max_to_receive> <time_between_log> <vlen> <src_IP> <src_port> <nb_parsers> <socket_buff_size> <output_q_size> <parser_q_size>
    msg_max_log->max_to_receive = atoi(argv[1]);
    msg_max_log->time_between_log = atoi(argv[2]);
    options->recvmmsg_vlen = atoi(argv[3]);
    options->address = argv[4];
    options->port = atoi(argv[5]);
    options->nb_parsers = atoi(argv[6]);
    options->socket_buff_size = atoi(argv[7]);
    options->output_queue_size = atoi(argv[8]);
    options->parsers_queue_size = atoi(argv[9]);
    printf("PARAMS: %d|%d|%d|%s|%d|%d|%ld|%d|%d\n", msg_max_log->max_to_receive, msg_max_log->time_between_log, 
      options->recvmmsg_vlen, options->address, options->port, options->nb_parsers, options->socket_buff_size,
      options->output_queue_size, options->parsers_queue_size);
  }
  else
  {
    printf("Using default values\n");
  }
}
struct thread_input
{
  queue_t *queue;
  int *count_total;
  int *received_msgs;
  struct messages_max_log *msg_max_log;
  int type_count; // 0 = generator_id | 1 = msg_id
};

void *t_read(void *in)
{
  struct thread_input *input = (struct thread_input *)in;
  int count_thread = 0;
  struct messages_max_log *msg_max = input->msg_max_log;
  int first = 1;
  int *received_msgs = input->received_msgs;
  queue_t *queue = input->queue;
  struct timespec start;
  struct timespec stop;
  struct timespec diff;
  pthread_t thread_id = pthread_self();
  uint32_t msg_id = 0;
  // printf("Thread: %ld\n", pthread_self());
  while (*input->count_total < input->msg_max_log->max_to_receive)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(queue);

    if (count_thread % (msg_max->time_between_log) == 0)
    {
      if (first)
      {
        first = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
      }
      else
      {
        clock_gettime(CLOCK_MONOTONIC, &stop);
        time_diff(&diff, &stop, &start, count_thread, thread_id);
        clock_gettime(CLOCK_MONOTONIC, &start);
      }
    }
    count_thread++;
    (*input->count_total)++;

    if (input->type_count == 0) {
      msg_id = seg->header->generator_id;
    } else {
      msg_id = seg->header->message_id;
    }

    // break when receive last_gen_id from scapy
    if (msg_id == LAST_GEN_ID)
    {
      fflush(stdout);
      unyte_free_all(seg);
      break;
    }

    received_msgs[msg_id] = 1;

    // printHeader(seg->header, stdout);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
    // printf("counter : %d\n", recv_count);
    fflush(stdout);

    /* Struct frees */
    unyte_free_all(seg);
  }

  return 0;
}

struct collector_threads
{
  pthread_t *threads;
  uint count;
};

struct collector_threads *create_collectors(uint count, struct thread_input *th_input)
{
  struct collector_threads *readers = (struct collector_threads *)malloc(sizeof(struct collector_threads));

  pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * count);
  pthread_t *cur = threads;
  for (uint i = 0; i < count; i++)
  {
    pthread_create(cur, NULL, t_read, (void *)th_input);
    cur++;
  }
  readers->count = count;
  readers->threads = threads;
  return readers;
}

void join_collectors(struct collector_threads *threads)
{
  pthread_t *cur = threads->threads;
  for (uint i = 0; i < threads->count; i++)
  {
    pthread_join(*cur, NULL);
    cur++;
  }
}

void clean_collector_threads(struct collector_threads *threads)
{
  free(threads->threads);
  free(threads);
}

int main(int argc, char *argv[])
{
  // Initialize collector options
  unyte_options_t options = {0};

  struct messages_max_log *msg_max = (struct messages_max_log *)malloc(sizeof(struct messages_max_log));

  parse_arguments(&options, msg_max, argc, argv);

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector(&options);
  int recv_count = 0;

  int received_msgs[msg_max->max_to_receive];
  memset(received_msgs, -1, msg_max->max_to_receive * sizeof(int));

  struct thread_input *th_input = malloc(sizeof(struct thread_input));
  th_input->count_total = &recv_count;
  th_input->msg_max_log = msg_max;
  th_input->queue = collector->queue;
  th_input->received_msgs = received_msgs;
  th_input->type_count = 0;

  struct collector_threads *collectors = create_collectors(NB_THREADS, th_input);
  join_collectors(collectors);
  clean_collector_threads(collectors);

  printf("Shutdown the socket\n");
  shutdown(*collector->sockfd, SHUT_RDWR);
  close(*collector->sockfd);
  pthread_join(*collector->main_thread, NULL);
  uint32_t msg_id = 0;
  /* Free last packets in the queue */
  while (is_queue_empty(collector->queue) != 0)
  {
    unyte_seg_met_t *seg = (unyte_seg_met_t *)unyte_queue_read(collector->queue);
    if (th_input->type_count == 0) {
      msg_id = seg->header->generator_id;
    } else {
      msg_id = seg->header->message_id;
    }
    if (msg_id != LAST_GEN_ID)
    {
      received_msgs[msg_id] = 1;
    }
    unyte_free_all(seg);
  }

  int lost_messages = 0;
  for (int i = 0; i < msg_max->max_to_receive; i++)
  {
    if (received_msgs[i] == -1)
    {
      lost_messages += 1;
    }
  }
  printf("Lost;%d/%d;%f\n", lost_messages, msg_max->max_to_receive, ((double)lost_messages / (double)msg_max->max_to_receive) * 100);

  // freeing collector mallocs
  unyte_free_collector(collector);
  fflush(stdout);

  free(msg_max);
  free(th_input);
  return 0;
}
