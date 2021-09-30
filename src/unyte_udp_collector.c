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
#include "unyte_udp_defaults.h"

void unyte_set_ip_headers_options(int socket_fd, sa_family_t family)
{
  int optval = 1;
  // get ip header IPv4
  if (setsockopt(socket_fd, IPPROTO_IP, IP_PKTINFO, &optval, sizeof(int)) < 0)
  {
    perror("Cannot set IP_PKTINFO option on socket");
    exit(EXIT_FAILURE);
  }
  // get ip header IPv6
  if (family == AF_INET6 && (setsockopt(socket_fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &optval, sizeof(int)) < 0))
  {
    perror("Cannot set IPV6_RECVPKTINFO option on socket");
    exit(EXIT_FAILURE);
  }
}

unyte_udp_sock_t *unyte_set_socket_opt(int socket_fd, bool msg_ip_dst)
{
  unyte_udp_sock_t *conn = (unyte_udp_sock_t *)malloc(sizeof(unyte_udp_sock_t));
  if (conn == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  conn->sockfd = (int *)malloc(sizeof(int));
  if (conn->sockfd == NULL)
  {
    free(conn);
    printf("Malloc failed\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_storage *sin = malloc(sizeof(struct sockaddr_storage));
  socklen_t len = sizeof(sin);
  if (getsockname(socket_fd, (struct sockaddr *)sin, &len) == -1)
  {
    perror("getsockname");
    exit(EXIT_FAILURE);
  }

  if (msg_ip_dst)
    unyte_set_ip_headers_options(socket_fd, ((struct sockaddr *)sin)->sa_family);

  conn->addr = sin;
  *conn->sockfd = socket_fd;
  return conn;
}

void set_default_options(unyte_udp_options_t *options)
{
  if (options == NULL)
  {
    printf("Invalid options.\n");
    exit(EXIT_FAILURE);
  }
  if (options->recvmmsg_vlen == 0)
    options->recvmmsg_vlen = UNYTE_DEFAULT_VLEN;
  if (options->output_queue_size <= 0)
    options->output_queue_size = OUTPUT_QUEUE_SIZE;
  if (options->nb_parsers <= 0)
    options->nb_parsers = DEFAULT_NB_PARSERS;
  if (options->parsers_queue_size <= 0)
    options->parsers_queue_size = PARSER_QUEUE_SIZE;
  if (options->monitoring_queue_size <= 0)
    options->monitoring_queue_size = MONITORING_QUEUE_SIZE;
  if (options->monitoring_delay <= 0)
    options->monitoring_delay = MONITORING_DELAY;
}

unyte_udp_collector_t *unyte_udp_create_listener(
    unyte_udp_queue_t *output_queue,
    unyte_udp_queue_t *monitoring_queue,
    unyte_udp_sock_t *conn,
    uint16_t vlen,
    uint nb_parsers,
    uint parsers_q_size,
    uint monitoring_delay,
    bool msg_dst_ip)
{
  pthread_t *udpListener = (pthread_t *)malloc(sizeof(pthread_t));
  struct listener_thread_input *listener_input = (struct listener_thread_input *)malloc(sizeof(struct listener_thread_input));
  unyte_udp_collector_t *collector = (unyte_udp_collector_t *)malloc((sizeof(unyte_udp_collector_t)));

  if (listener_input == NULL || udpListener == NULL || collector == NULL)
  {
    printf("Malloc failed.\n");
    return NULL;
  }

  listener_input->output_queue = output_queue;
  listener_input->monitoring_queue = monitoring_queue;
  listener_input->conn = conn;
  listener_input->recvmmsg_vlen = vlen;
  listener_input->nb_parsers = nb_parsers;
  listener_input->parser_queue_size = parsers_q_size;
  listener_input->monitoring_delay = monitoring_delay;
  listener_input->msg_dst_ip = msg_dst_ip;

  /*Threaded UDP listener*/
  pthread_create(udpListener, NULL, t_listener, (void *)listener_input);

  collector->queue = output_queue;
  collector->monitoring_queue = monitoring_queue;
  collector->sockfd = conn->sockfd;
  collector->main_thread = udpListener;

  return collector;
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

  unyte_udp_sock_t *conn = unyte_set_socket_opt(options->socket_fd, options->msg_dst_ip);

  /* Return struct */
  unyte_udp_collector_t *collector = unyte_udp_create_listener(
      output_queue, monitoring_queue, conn, options->recvmmsg_vlen, options->nb_parsers,
      options->parsers_queue_size, options->monitoring_delay, options->msg_dst_ip);

  if (collector == NULL)
  {
    printf("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

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

int unyte_udp_free_options(unyte_option_t *options)
{
  unyte_option_t *head = options;
  unyte_option_t *cur = options->next;
  unyte_option_t *to_rm;
  while (cur != NULL)
  {
    free(cur->data);
    to_rm = cur;
    cur = cur->next;
    free(to_rm);
  }
  free(head);
  return 0;
}

int unyte_udp_free_header(unyte_seg_met_t *seg)
{
  unyte_udp_free_options(seg->header->options);
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
