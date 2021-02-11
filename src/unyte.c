#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "queue.h"
#include "unyte.h"
#include "unyte_utils.h"
#include "listening_worker.h"

/**
 * Not exposed function used to initialize the socket and return unyte_socket struct
 */
unyte_sock_t *unyte_init_socket(char *addr, uint16_t port)
{
  unyte_sock_t *conn = (unyte_sock_t *)malloc(sizeof(unyte_sock_t));
  if (conn == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  /* TODO put in constant */
  int release = 1;
  struct sockaddr_in *adresse = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

  /*create socket on UDP protocol*/
  int *sock = (int *)malloc(sizeof(int));
  *sock = socket(AF_INET, SOCK_DGRAM, 0);

  /*handle error*/
  if (*sock < 0)
  {
    perror("Cannot create socket\n");
    exit(EXIT_FAILURE);
  }

  // TODO: SO_REUSEPORT = 15 but compilation error with gcc
  setsockopt(*sock, SOL_SOCKET, 15, &release, sizeof(int));

  adresse->sin_family = AF_INET;
  adresse->sin_port = htons(port);
  inet_pton(AF_INET, addr, &adresse->sin_addr);

  if (bind(*sock, (struct sockaddr *)adresse, sizeof(*adresse)) == -1)
  {
    perror("Bind failed");
    close(*sock);
    exit(EXIT_FAILURE);
  }

  conn->addr = adresse;
  conn->sockfd = sock;

  return conn;
}

void set_default_options(unyte_options_t *options)
{
  if (options == NULL)
  {
    printf("Invalid options.\n");
    exit(EXIT_FAILURE);
  }
  if (options->address == NULL)
  {
    printf("Invalid address.\n");
    exit(EXIT_FAILURE);
  }
  if (options->recvmmsg_vlen == 0)
  {
    options->recvmmsg_vlen = DEFAULT_VLEN;
  }
  printf("Options: %s:%d | vlen: %d\n", options->address, options->port, options->recvmmsg_vlen);
}

/**
 * Start all the subprocesses of the collector on the given port and return the output segment queue.

 * Messages in the queues are structured in structs unyte_segment_with_metadata like defined in 
 * unyte_utils.h.
 */
unyte_collector_t *unyte_start_collector(unyte_options_t *options)
{
  set_default_options(options);

  queue_t *output_queue = (queue_t *)malloc(sizeof(queue_t));
  if (output_queue == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  /* Filling queue and creating thread mem protections. */
  output_queue->head = 0;
  output_queue->tail = 0;
  output_queue->size = OUTPUT_QUEUE_SIZE;
  output_queue->data = malloc(sizeof(void *) * OUTPUT_QUEUE_SIZE);
  sem_init(&output_queue->empty, 0, OUTPUT_QUEUE_SIZE);
  sem_init(&output_queue->full, 0, 0);
  pthread_mutex_init(&output_queue->lock, NULL);

  pthread_t *udpListener = (pthread_t *)malloc(sizeof(pthread_t));
  if (udpListener == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  unyte_sock_t *conn = unyte_init_socket(options->address, options->port);

  struct listener_thread_input *listener_input = (struct listener_thread_input *)malloc(sizeof(struct listener_thread_input));
  if (listener_input == NULL)
  {
    printf("Malloc failed.\n");
    exit(-1);
  }

  listener_input->port = options->port;
  listener_input->output_queue = output_queue;
  listener_input->conn = conn;
  listener_input->recvmmsg_vlen = options->recvmmsg_vlen;

  /*Threaded UDP listener*/
  pthread_create(udpListener, NULL, t_listener, (void *)listener_input);

  /* Return struct */
  unyte_collector_t *collector = (unyte_collector_t *)malloc((sizeof(unyte_collector_t)));
  if (collector == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  collector->queue = output_queue;
  collector->sockfd = conn->sockfd;
  collector->main_thread = udpListener;

  return collector;
}

/**
 * Free all the mem related to the segment
 */
int unyte_free_all(unyte_seg_met_t *seg)
{
  /* Free all the sub modules */

  free(seg->payload);
  free(seg->header);
  free(seg->metadata);

  /* Free the struct itself */

  free(seg);

  return 0;
}

/**
 * Free only the payload
 * pointer still exist but is NULL
 */
int unyte_free_payload(unyte_seg_met_t *seg)
{
  free(seg->payload);
  return 0;
}

/**
 * Free only the header
 * pointer still exist but is NULL
 */
int unyte_free_header(unyte_seg_met_t *seg)
{
  free(seg->header);
  return 0;
}

/**
 * Free only the metadata
 * pointer still exist but is NULL
 */
int unyte_free_metadata(unyte_seg_met_t *seg)
{
  free(seg->metadata);
  return 0;
}
