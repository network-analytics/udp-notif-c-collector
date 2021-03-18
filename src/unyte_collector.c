#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "unyte_collector.h"
#include "listening_worker.h"

/**
 * Not exposed function used to initialize the socket and return unyte_socket struct
 */
unyte_sock_t *unyte_init_socket(char *addr, uint16_t port, uint64_t sock_buff_size)
{
  unyte_sock_t *conn = (unyte_sock_t *)malloc(sizeof(unyte_sock_t));
  if (conn == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

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

  int optval = 1;
  setsockopt(*sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int));

  uint64_t receive_buf_size;
  if (sock_buff_size <= 0) {
    receive_buf_size = DEFAULT_SK_BUFF_SIZE; // 20MB
  } else {
    receive_buf_size = sock_buff_size;
  }
  setsockopt(*sock, SOL_SOCKET, SO_RCVBUF, &receive_buf_size, sizeof(receive_buf_size));

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
  if (options->output_queue_size == 0) {
    options->output_queue_size = OUTPUT_QUEUE_SIZE;
  }
  if (options->nb_parsers == 0) {
    options->nb_parsers = DEFAULT_NB_PARSERS;
  }
  if (options->parsers_queue_size == 0) {
    options->parsers_queue_size = PARSER_QUEUE_SIZE;
  }
  // printf("Options: %s:%d | vlen: %d|outputqueue: %d| parsers:%d\n", options->address, options->port, options->recvmmsg_vlen, options->output_queue_size, options->nb_parsers);
  // printf("Options: parser_queue_size:%d\n", options->parsers_queue_size);
}

unyte_collector_t *unyte_start_collector(unyte_options_t *options)
{
  set_default_options(options);

  queue_t *output_queue = unyte_queue_init(options->output_queue_size);
  if (output_queue == NULL) {
    // malloc errors
    exit(EXIT_FAILURE);
  }

  pthread_t *udpListener = (pthread_t *)malloc(sizeof(pthread_t));
  if (udpListener == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  unyte_sock_t *conn = unyte_init_socket(options->address, options->port, options->socket_buff_size);

  struct listener_thread_input *listener_input = (struct listener_thread_input *)malloc(sizeof(struct listener_thread_input));
  if (listener_input == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  listener_input->port = options->port;
  listener_input->output_queue = output_queue;
  listener_input->conn = conn;
  listener_input->recvmmsg_vlen = options->recvmmsg_vlen;
  listener_input->nb_parsers = options->nb_parsers;
  listener_input->parser_queue_size = options->parsers_queue_size;

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

int unyte_free_payload(unyte_seg_met_t *seg)
{
  free(seg->payload);
  return 0;
}

int unyte_free_header(unyte_seg_met_t *seg)
{
  free(seg->header);
  return 0;
}

int unyte_free_metadata(unyte_seg_met_t *seg)
{
  free(seg->metadata);
  return 0;
}

int unyte_free_collector(unyte_collector_t *collector)
{
  free(collector->queue->data);
  free(collector->queue);
  free(collector->main_thread);
  free(collector);
  return 0;
}
