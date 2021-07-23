#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <string.h>
#include "unyte_udp_collector.h"
#include "listening_worker.h"
#include "unyte_version.h"

/**
 * Not exposed function used to initialize the socket and return unyte_socket struct
 */
unyte_udp_sock_t *unyte_init_socket(char *addr, char *port, uint64_t sock_buff_size)
{
  struct addrinfo *addr_info;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

  int rc = getaddrinfo(addr, port, &hints, &addr_info);

  if (rc != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(rc));
    exit(EXIT_FAILURE);
  }

  unyte_udp_sock_t *conn = (unyte_udp_sock_t *)malloc(sizeof(unyte_udp_sock_t));
  int *sock = (int *)malloc(sizeof(int));
  struct sockaddr_storage *address = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
  printf("Address type: %s | %d\n", (addr_info->ai_family == AF_INET) ? "IPv4" : "IPv6", ntohs(((struct sockaddr_in *)addr_info->ai_addr)->sin_port));

  if (conn == NULL || address == NULL || sock == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  // safe a copy of the bound addr
  memset(address, 0, sizeof(*address));
  memcpy(address, addr_info->ai_addr, addr_info->ai_addrlen);

  /*create socket on UDP protocol*/
  *sock = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);

  /*handle error*/
  if (*sock < 0)
  {
    perror("Cannot create socket");
    exit(EXIT_FAILURE);
  }

  int optval = 1;
  if (setsockopt(*sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int)) < 0)
  {
    perror("Cannot set SO_REUSEPORT option on socket");
    exit(EXIT_FAILURE);
  }

  // get ip header IPv4
  if (setsockopt(*sock, IPPROTO_IP, IP_PKTINFO, &optval, sizeof(int)) < 0)
  {
    perror("Cannot set IP_PKTINFO option on socket");
    exit(EXIT_FAILURE);
  }

  // get ip header IPv6
  if (addr_info->ai_family == AF_INET6 && (setsockopt(*sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &optval, sizeof(int)) < 0))
  {
    perror("Cannot set IPV6_RECVPKTINFO option on socket");
    exit(EXIT_FAILURE);
  }

  // get IPv4 in header IPv6
  // int off = 0;
  // if (setsockopt(*sock, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(int)) < 0)
  // {
  //   perror("Cannot set IPV6_V6ONLY to 0 option on socket");
  //   exit(EXIT_FAILURE);
  // }

  uint64_t receive_buf_size;
  if (sock_buff_size <= 0)
    receive_buf_size = DEFAULT_SK_BUFF_SIZE; // 20MB
  else
    receive_buf_size = sock_buff_size;

  if (setsockopt(*sock, SOL_SOCKET, SO_RCVBUF, &receive_buf_size, sizeof(receive_buf_size)) < 0)
  {
    perror("Cannot set buffer size");
    exit(EXIT_FAILURE);
  }

  if (bind(*sock, addr_info->ai_addr, (int)addr_info->ai_addrlen) == -1)
  {
    perror("Bind failed");
    close(*sock);
    exit(EXIT_FAILURE);
  }

  // free addr_info after usage
  freeaddrinfo(addr_info);

  conn->addr = address;
  conn->sockfd = sock;

  return conn;
}

void set_default_options(unyte_udp_options_t *options)
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
  if (options->output_queue_size <= 0)
  {
    options->output_queue_size = OUTPUT_QUEUE_SIZE;
  }
  if (options->nb_parsers <= 0)
  {
    options->nb_parsers = DEFAULT_NB_PARSERS;
  }
  if (options->parsers_queue_size <= 0)
  {
    options->parsers_queue_size = PARSER_QUEUE_SIZE;
  }
  if (options->monitoring_queue_size <= 0)
  {
    options->monitoring_queue_size = MONITORING_QUEUE_SIZE;
  }
  if (options->monitoring_delay <= 0)
  {
    options->monitoring_delay = MONITORING_DELAY;
  }
  // printf("Options: %s:%d | vlen: %d|outputqueue: %d| parsers:%d\n", options->address, options->port, options->recvmmsg_vlen, options->output_queue_size, options->nb_parsers);
  // printf("Options: parser_queue_size:%d\n", options->parsers_queue_size);
}

unyte_udp_collector_t *unyte_udp_start_collector(unyte_udp_options_t *options)
{
  set_default_options(options);

  unyte_udp_queue_t *output_queue = unyte_udp_queue_init(options->output_queue_size);
  unyte_udp_queue_t *monitoring_queue = unyte_udp_queue_init(options->monitoring_queue_size);

  if (output_queue == NULL || monitoring_queue == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  pthread_t *udpListener = (pthread_t *)malloc(sizeof(pthread_t));
  if (udpListener == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  unyte_udp_sock_t *conn = unyte_init_socket(options->address, options->port, options->socket_buff_size);

  struct listener_thread_input *listener_input = (struct listener_thread_input *)malloc(sizeof(struct listener_thread_input));
  if (listener_input == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  listener_input->output_queue = output_queue;
  listener_input->monitoring_queue = monitoring_queue;
  listener_input->conn = conn;
  listener_input->recvmmsg_vlen = options->recvmmsg_vlen;
  listener_input->nb_parsers = options->nb_parsers;
  listener_input->parser_queue_size = options->parsers_queue_size;
  listener_input->monitoring_delay = options->monitoring_delay;

  /*Threaded UDP listener*/
  pthread_create(udpListener, NULL, t_listener, (void *)listener_input);

  /* Return struct */
  unyte_udp_collector_t *collector = (unyte_udp_collector_t *)malloc((sizeof(unyte_udp_collector_t)));
  if (collector == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  collector->queue = output_queue;
  collector->monitoring_queue = monitoring_queue;
  collector->sockfd = conn->sockfd;
  collector->main_thread = udpListener;

  return collector;
}

int unyte_udp_free_all(unyte_seg_met_t *seg)
{
  /* Free all the sub modules */
  unyte_udp_free_payload(seg);
  unyte_udp_free_header(seg);
  unyte_udp_free_metadata(seg);

  /* Free the struct itself */
  free(seg);

  return 0;
}

int unyte_udp_free_payload(unyte_seg_met_t *seg)
{
  free(seg->payload);
  return 0;
}

int unyte_udp_free_header(unyte_seg_met_t *seg)
{
  free(seg->header);
  return 0;
}

int unyte_udp_free_metadata(unyte_seg_met_t *seg)
{
  free(seg->metadata->src);
  free(seg->metadata->dest);
  free(seg->metadata);
  return 0;
}

int unyte_udp_free_collector(unyte_udp_collector_t *collector)
{
  // Freeing last collector queue
  while (is_udp_queue_empty(collector->queue) != 0)
    unyte_udp_free_all((unyte_seg_met_t *)unyte_udp_queue_read(collector->queue));

  // Freeing last monitoring counter queue
  while (is_udp_queue_empty(collector->monitoring_queue) != 0)
    free(unyte_udp_queue_read(collector->monitoring_queue));

  free(collector->queue->data);
  free(collector->queue);
  free(collector->monitoring_queue->data);
  free(collector->monitoring_queue);
  free(collector->main_thread);
  free(collector);
  return 0;
}

int get_int_len(int value)
{
  int l = 1;
  while (value > 9)
  {
    l++;
    value /= 10;
  }
  return l;
}

char *unyte_udp_notif_version()
{
  int major_len = get_int_len(UNYTE_UDP_NOTIF_VERSION_MAJOR);
  int minor_len = get_int_len(UNYTE_UDP_NOTIF_VERSION_MINOR);
  int patch_len = get_int_len(UNYTE_UDP_NOTIF_VERSION_PATCH);
  uint len = major_len + minor_len + patch_len + 3; // 2 points and 1 end of string
  char *version = (char *)malloc(len * sizeof(char));
  sprintf(version, "%d.%d.%d", UNYTE_UDP_NOTIF_VERSION_MAJOR, UNYTE_UDP_NOTIF_VERSION_MINOR, UNYTE_UDP_NOTIF_VERSION_PATCH);
  return version;
}
