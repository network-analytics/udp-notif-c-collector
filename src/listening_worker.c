#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include "listening_worker.h"
#include "segmentation_buffer.h"
#include "parsing_worker.h"
#include "unyte_udp_collector.h"
#include "cleanup_worker.h"
#include "unyte_pooling.h"

void stop_parsers_and_monitoring(struct parse_worker *parsers, struct listener_thread_input *in, struct monitoring_worker *monitoring)
{
  for (uint i = 0; i < in->nb_parsers; i++)
  {
    // stopping all clean up thread first
    (parsers + i)->cleanup_worker->cleanup_in->stop_cleanup_thread = 1;
  }
  monitoring->monitoring_in->stop_monitoring_thread = true;
}

/**
 * Frees all parsers mallocs and thread input memory.
 */
void free_parsers(struct parse_worker *parsers, struct listener_thread_input *in, struct mmsghdr *messages)
{
  /* Kill every workers here */
  for (uint i = 0; i < in->nb_parsers; i++)
  {
    // Kill clean up thread
    pthread_join(*(parsers + i)->cleanup_worker->cleanup_thread, NULL);
    free((parsers + i)->cleanup_worker->cleanup_thread);
    free((parsers + i)->cleanup_worker);

    // Kill worker thread
    pthread_cancel(*(parsers + i)->worker);
    pthread_join(*(parsers + i)->worker, NULL);
    free((parsers + i)->queue->data);
    free((parsers + i)->worker);
    free((parsers + i)->queue);
    clear_buffer(NULL, (parsers + i)->input->segment_buff);
    unyte_free_pool((parsers + i)->input->pool);
    free((parsers + i)->input);
  }

  free(parsers);

  free(in->conn->sockfd);
  free(in->conn->addr);
  free(in->conn);

  for (uint16_t i = 0; i < in->recvmmsg_vlen; i++)
  {
    free(messages[i].msg_hdr.msg_iov->iov_base);
    free(messages[i].msg_hdr.msg_iov);
    free(messages[i].msg_hdr.msg_name);
  }
  free(messages);
}

void free_monitoring_worker(struct monitoring_worker *monitoring)
{
  if (monitoring->running)
    pthread_join(*monitoring->monitoring_thread, NULL);
  free(monitoring->monitoring_thread);
  unyte_udp_free_seg_counters(monitoring->monitoring_in->counters, monitoring->monitoring_in->nb_counters);
  free(monitoring->monitoring_in);
  free(monitoring);
}

/**
 * Creates a thread with a cleanup cron worker
 */
int create_cleanup_thread(struct segment_buffer *seg_buff, struct parse_worker *parser)
{
  pthread_t *clean_up_thread = (pthread_t *)malloc(sizeof(pthread_t));
  struct cleanup_thread_input *cleanup_in = (struct cleanup_thread_input *)malloc(sizeof(struct cleanup_thread_input));
  parser->cleanup_worker = (struct cleanup_worker *)malloc(sizeof(struct cleanup_worker));

  if (clean_up_thread == NULL || cleanup_in == NULL || parser->cleanup_worker == NULL)
  {
    printf("Malloc failed.\n");
    return -1;
  }

  cleanup_in->seg_buff = seg_buff;
  cleanup_in->time = CLEANUP_FLAG_CRON;
  cleanup_in->stop_cleanup_thread = 0;

  pthread_create(clean_up_thread, NULL, t_clean_up, (void *)cleanup_in);

  // Saving cleanup_worker references for free afterwards
  parser->cleanup_worker->cleanup_thread = clean_up_thread;
  parser->cleanup_worker->cleanup_in = cleanup_in;

  return 0;
}
/**
 * Creates a thread with a parse worker.
 */
int create_parse_worker(struct parse_worker *parser, struct listener_thread_input *in, unyte_seg_counters_t *counters, int monitoring_running)
{
  parser->queue = unyte_udp_queue_init(in->parser_queue_size);
  if (parser->queue == NULL)
  {
    // malloc failed
    return -1;
  }

  parser->worker = (pthread_t *)malloc(sizeof(pthread_t));
  // Define the struct passed to the next parser
  struct parser_thread_input *parser_input = (struct parser_thread_input *)malloc(sizeof(struct parser_thread_input));
  if (parser->worker == NULL || parser_input == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  /* Fill the struct */
  parser_input->input = parser->queue;
  parser_input->output = in->output_queue;
  parser_input->segment_buff = create_segment_buffer();
  parser_input->counters = counters;
  parser_input->counters->type = PARSER_WORKER;
  for (uint i = 0; i < ACTIVE_GIDS; i++)
  {
    (parser_input->counters->active_gids + i)->active = 0;
  }

  parser_input->monitoring_running = monitoring_running;
  parser_input->pool = unyte_pool_init(sizeof(struct message_segment_list_cell));

  if (parser_input->segment_buff == NULL || parser_input->counters->active_gids == NULL)
  {
    printf("Create segment buffer failed.\n");
    return -1;
  }

  /* Create the thread */
  pthread_create(parser->worker, NULL, t_parser, (void *)parser_input);

  /* Store the pointer to be able to free it at the end */
  parser->input = parser_input;

  return create_cleanup_thread(parser_input->segment_buff, parser);
}

int create_monitoring_thread(struct monitoring_worker *monitoring, unyte_udp_queue_t *out_mnt_queue, uint delay, unyte_seg_counters_t *counters, uint nb_counters)
{
  struct monitoring_thread_input *mnt_input = (struct monitoring_thread_input *)malloc(sizeof(struct monitoring_thread_input));

  pthread_t *th_monitoring = (pthread_t *)malloc(sizeof(pthread_t));

  if (mnt_input == NULL || th_monitoring == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  mnt_input->delay = delay;
  mnt_input->output_queue = out_mnt_queue;
  mnt_input->counters = counters;
  mnt_input->nb_counters = nb_counters;
  mnt_input->stop_monitoring_thread = false;

  if (out_mnt_queue->size != 0)
  {
    /* Create the thread */
    pthread_create(th_monitoring, NULL, t_monitoring_unyte_udp, (void *)mnt_input);
    monitoring->running = true;
  }
  else
  {
    monitoring->running = false;
  }

  /* Store the pointer to be able to free it at the end */
  monitoring->monitoring_in = mnt_input;
  monitoring->monitoring_thread = th_monitoring;

  return 0;
}

/**
 * Udp listener worker on PORT port.
 */
int listener(struct listener_thread_input *in)
{
  /* errno used to handle socket read errors */
  errno = 0;

  /* Create parsing workers */
  struct parse_worker *parsers = malloc(sizeof(struct parse_worker) * in->nb_parsers);
  if (parsers == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  struct monitoring_worker *monitoring = malloc(sizeof(struct monitoring_worker));
  if (monitoring == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  uint nb_counters = in->nb_parsers + 1;
  unyte_seg_counters_t *counters = unyte_udp_init_counters(nb_counters); // parsers + listening workers

  if (counters == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  int monitoring_ret = create_monitoring_thread(monitoring, in->monitoring_queue, in->monitoring_delay, counters, nb_counters);
  if (monitoring_ret < 0)
    return monitoring_ret;

  for (uint i = 0; i < in->nb_parsers; i++)
  {
    int created = create_parse_worker((parsers + i), in, (monitoring->monitoring_in->counters + i), monitoring->running);
    if (created < 0)
      return created;
  }

  struct mmsghdr *messages = (struct mmsghdr *)malloc(in->recvmmsg_vlen * sizeof(struct mmsghdr));
  if (messages == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }
  // Init msg_iov first to reduce mallocs every iteration of while
  for (uint16_t i = 0; i < in->recvmmsg_vlen; i++)
  {
    messages[i].msg_hdr.msg_iov = (struct iovec *)malloc(sizeof(struct iovec));
    messages[i].msg_hdr.msg_iovlen = 1;
  }
  // FIXME: malloc failed
  // TODO: malloc array of msg_hdr.msg_name and assign addresss to every messages[i]
  unyte_seg_counters_t *listener_counter = monitoring->monitoring_in->counters + in->nb_parsers;
  listener_counter->thread_id = pthread_self();
  listener_counter->type = LISTENER_WORKER;
  while (1)
  {
    for (uint16_t i = 0; i < in->recvmmsg_vlen; i++)
    {
      messages[i].msg_hdr.msg_iov->iov_base = (char *)malloc(UDP_SIZE * sizeof(char));
      messages[i].msg_hdr.msg_iov->iov_len = UDP_SIZE;
      messages[i].msg_hdr.msg_control = 0;
      messages[i].msg_hdr.msg_controllen = 0;
      messages[i].msg_hdr.msg_name = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
      messages[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
    }

    int read_count = recvmmsg(*in->conn->sockfd, messages, in->recvmmsg_vlen, 0, NULL);
    if (read_count == -1)
    {
      perror("recvmmsg failed");
      close(*in->conn->sockfd);
      stop_parsers_and_monitoring(parsers, in, monitoring);
      free_parsers(parsers, in, messages);
      free_monitoring_worker(monitoring);
      return -1;
    }

    for (int i = 0; i < read_count; i++)
    {
      // If msg_len == 0 -> message has 0 bytes -> we discard message and free the buffer
      if (messages[i].msg_len > 0)
      {
        unyte_min_t *seg = minimal_parse(messages[i].msg_hdr.msg_iov->iov_base, ((struct sockaddr_in *)messages[i].msg_hdr.msg_name), in->conn->addr);
        if (seg == NULL)
        {
          printf("minimal_parse error\n");
          return -1;
        }
        /* Dispatching by modulo on threads */
        uint32_t seg_gid = seg->generator_id;
        uint32_t seg_mid = seg->message_id;
        int ret = unyte_udp_queue_write((parsers + (seg->generator_id % in->nb_parsers))->queue, seg);
        // if ret == -1 --> queue is full, we discard message
        if (ret < 0)
        {
          // printf("1.losing message on parser queue\n");
          if (monitoring->running)
            unyte_udp_update_dropped_segment(listener_counter, seg_gid, seg_mid);
          //TODO: syslog + count stat
          free(seg->buffer);
          free(seg);
        }
        else if (monitoring->running)
        {
          unyte_udp_update_received_segment(listener_counter, seg_gid, seg_mid);
        }
      }
      else
      {
        free(messages[i].msg_hdr.msg_iov->iov_base);
      }
    }

    for (uint16_t i = 0; i < in->recvmmsg_vlen; i++)
    {
      free(messages[i].msg_hdr.msg_name);
    }
  }

  // Never called cause while(1)
  for (uint16_t i = 0; i < in->recvmmsg_vlen; i++)
  {
    free(messages[i].msg_hdr.msg_iov);
  }
  free(messages);

  return 0;
}

/**
 * Threadified app functin listening on *P port.
 */
void *t_listener(void *in)
{
  listener((struct listener_thread_input *)in);
  free(in);
  pthread_exit(NULL);
}
