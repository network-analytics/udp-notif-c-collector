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
#include "parsing_worker.h"
#include "hexdump.h"
#include "unyte_utils.h"
#include "unyte.h"
#include "queue.h"

#define RCVSIZE 65535
#define QUEUE_SIZE 50
#define PACKETS_VLEN 10

struct parse_worker
{
  queue_t *queue;
  pthread_t *worker;
  struct parser_thread_input *input;
};

void free_parsers(struct parse_worker *parsers, struct listener_thread_input *in, struct mmsghdr *messages)
{

  /* Kill every workers here */
  for (int i = 0; i < PARSER_NUMBER; i++)
  {
    pthread_cancel(*(parsers + i)->worker);
    pthread_join(*(parsers + i)->worker, NULL);
    free((parsers + i)->queue->data);
    free((parsers + i)->worker);
    free((parsers + i)->queue);
    free((parsers + i)->input);
  }

  free(parsers);

  free(in->conn->sockfd);
  free(in->conn->addr);
  free(in->conn);

  for (int i = 0; i < PACKETS_VLEN; i++)
  {
    free(messages[i].msg_hdr.msg_iov->iov_base);
    free(messages[i].msg_hdr.msg_iov);
    free(messages[i].msg_hdr.msg_name);
  }
  free(messages);
}

/**
 * Udp listener worker on PORT port.
 */
int listener(struct listener_thread_input *in)
{
  /* errno used to handle socket read errors */
  errno = 0;

  /* Iterations number if not commented in the while loop */
  int infinity = 10;

  /* Create parsing workers */
  struct parse_worker *parsers = malloc(sizeof(struct parse_worker) * PARSER_NUMBER);
  if (parsers == NULL)
  {
    printf("Malloc failed \n");
    return -1;
  }

  for (int i = 0; i < PARSER_NUMBER; i++)
  {
    (parsers + i)->queue = (queue_t *)malloc(sizeof(queue_t));
    if ((parsers + i)->queue == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    /* Filling queue and creating thread mem protections. */
    (parsers + i)->queue->head = 0;
    (parsers + i)->queue->tail = 0;
    (parsers + i)->queue->size = QUEUE_SIZE;
    (parsers + i)->queue->data = malloc(sizeof(void *) * QUEUE_SIZE);
    sem_init(&(parsers + i)->queue->empty, 0, QUEUE_SIZE);
    sem_init(&(parsers + i)->queue->full, 0, 0);
    pthread_mutex_init(&(parsers + i)->queue->lock, NULL);

    if ((parsers + i)->queue->data == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    (parsers + i)->worker = (pthread_t *)malloc(sizeof(pthread_t));
    if ((parsers + i)->worker == NULL)
    {
      printf("Malloc failed \n");
      return -1;
    }

    /* Define the struct passed to the next parser */
    struct parser_thread_input *parser_input = (struct parser_thread_input *)malloc(sizeof(struct parser_thread_input));
    if (parser_input == NULL)
    {
      printf("Malloc failed.\n");
      exit(-1);
    }

    /* Fill the struct */
    parser_input->input = (parsers + i)->queue;
    parser_input->output = in->output_queue;

    /* Create the thread */
    pthread_create((parsers + i)->worker, NULL, t_parser, (void *)parser_input);

    /* Store the pointer to be able to free it at the end */
    (parsers + i)->input = parser_input;
  }

  /* Uncomment if no listening is wanted */
  /* infinity = 0; */

  while (infinity > 0)
  {
    // TODO: init only once
    struct mmsghdr *messages = (struct mmsghdr *)malloc(PACKETS_VLEN * sizeof(struct mmsghdr));
    for (size_t i = 0; i < PACKETS_VLEN; i++)
    {
      messages[i].msg_hdr.msg_iov = (struct iovec *)malloc(sizeof(struct iovec));
      messages[i].msg_hdr.msg_iovlen = 1;
      messages[i].msg_hdr.msg_iov->iov_base = (char *)malloc(RCVSIZE * sizeof(char));
      messages[i].msg_hdr.msg_iov->iov_len = RCVSIZE;
      messages[i].msg_hdr.msg_control = 0;
      messages[i].msg_hdr.msg_controllen = 0;
      messages[i].msg_hdr.msg_name = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
      messages[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
    }

    int read_count = recvmmsg(*in->conn->sockfd, messages, PACKETS_VLEN, 0, NULL);
    //TODO: check messages[i].msg_hdr.msg_flags & MSG_TRUNC --> if true datagram.len>buffer

    printf("%d messages read\n", read_count);
    for (long int i = 0; i < read_count; i++)
    {
      // hexdump(messages[i].msg_hdr.msg_iov->iov_base, 10000);

      // If msg_len == 0 -> message has 0 bytes -> we discard message and free the buffer
      if (messages[i].msg_len > 0) {
        unyte_min_t *seg = minimal_parse(messages[i].msg_hdr.msg_iov->iov_base, ((struct sockaddr_in *)messages[i].msg_hdr.msg_name), in->conn->addr);
        /* Dispatching by modulo on threads */
        unyte_queue_write((parsers + (seg->generator_id % PARSER_NUMBER))->queue, seg);
      } else {
        free(messages[i].msg_hdr.msg_iov->iov_base);
      }
    }
    if (read_count == -1)
    {
      perror("recvmmsg failed");
      close(*in->conn->sockfd);
      free_parsers(parsers, in, messages);
      return -1;
    }

    for (int i = 0; i < PACKETS_VLEN; i++)
    {
      free(messages[i].msg_hdr.msg_iov);
      free(messages[i].msg_hdr.msg_name);
    }
    free(messages);

    /* Comment if infinity is required */
    /* infinity = infinity - 1; */
  }

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
