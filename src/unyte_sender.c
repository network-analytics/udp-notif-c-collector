#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "unyte_sender.h"
#include <net/if.h>

struct unyte_sender_socket *unyte_start_sender(unyte_sender_options_t *options)
{
  unyte_IP_type_t ip_type = get_IP_type(options->address);
  if (ip_type == unyte_UNKNOWN)
  {
    printf("Invalid IP address: %s\n", options->address);
    exit(EXIT_FAILURE);
  }

  void *addr;
  struct unyte_sender_socket *sender_sk = (struct unyte_sender_socket *)malloc(sizeof(struct unyte_sender_socket));

  if (ip_type == unyte_IPV4)
    addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
  else
    addr = (struct sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));

  if (addr == NULL || sender_sk == NULL)
  {
    perror("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  int sockfd;
  if (ip_type == unyte_IPV4)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  else
    sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

  if (sockfd < 0)
  {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  // Binding socket to an interface
  const char *interface = options->interface;
  if (options->interface != NULL && (strlen(interface) > 0))
  {
    printf("Setting interface: %s\n", interface);
    int len = strnlen(interface, IFNAMSIZ);
    if (len == IFNAMSIZ)
    {
      fprintf(stderr, "Too long iface name");
      exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface, len) < 0)
    {
      perror("Bind socket to interface failed");
      exit(EXIT_FAILURE);
    }
  }

  uint64_t send_buf_size = DEFAULT_SK_SND_BUFF_SIZE;
  if (options->socket_buff_size > 0)
    send_buf_size = options->socket_buff_size;

  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size)))
  {
    perror("Set socket buffer size");
    exit(EXIT_FAILURE);
  }

  int connect_ret = 0;
  if (ip_type == unyte_IPV4)
  {
    memset(addr, 0, sizeof(struct sockaddr_in));
    ((struct sockaddr_in *)addr)->sin_family = AF_INET;
    ((struct sockaddr_in *)addr)->sin_port = htons(options->port);
    inet_pton(AF_INET, options->address, &((struct sockaddr_in *)addr)->sin_addr);
    connect_ret = connect(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
  }
  else
  {
    memset(addr, 0, sizeof(struct sockaddr_in6));
    ((struct sockaddr_in6 *)addr)->sin6_family = AF_INET6;
    ((struct sockaddr_in6 *)addr)->sin6_port = htons(options->port);
    ((struct sockaddr_in6 *)addr)->sin6_scope_id = if_nametoindex("eth0"); //TODO: check all interfaces or bind to one interface
    ((struct sockaddr_in6 *)addr)->sin6_flowinfo = 0;
    inet_pton(AF_INET6, options->address, &((struct sockaddr_in6 *)addr)->sin6_addr);
    connect_ret = connect(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in6));
  }

  // connect socket to destination address
  if (connect_ret == -1)
  {
    perror("Connect failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  sender_sk->sock_in = addr;
  sender_sk->sockfd = sockfd;
  sender_sk->default_mtu = options->default_mtu;
  return sender_sk;
}

int unyte_send(struct unyte_sender_socket *sender_sk, unyte_message_t *message)
{
  uint mtu = message->used_mtu;
  if (mtu <= 0)
  {
    if (sender_sk->default_mtu > 0)
      mtu = sender_sk->default_mtu;
    else
      mtu = DEFAULT_MTU;
  }

  struct unyte_segmented_msg *packets = build_message(message, mtu);
  unyte_seg_met_t *current_seg = packets->segments;
  for (uint i = 0; i < packets->segments_len; i++)
  {
    unsigned char *parsed_packet = serialize_message(current_seg);
    int res_send = send(sender_sk->sockfd, parsed_packet, current_seg->header->header_length + current_seg->header->message_length, 0);

    if (res_send < 0)
    {
      // perror("send()");
    }
    free(parsed_packet);
    current_seg++;
  }
  free_seg_msgs(packets);

  return 0;
}

int free_sender_socket(struct unyte_sender_socket *sender_sk)
{
  free(sender_sk->sock_in);
  free(sender_sk);
  return 0;
}

int free_seg_msgs(struct unyte_segmented_msg *packets)
{
  unyte_seg_met_t *current = packets->segments;
  for (uint i = 0; i < packets->segments_len; i++)
  {
    free(current->payload);
    unyte_option_t *next = current->header->options->next;
    while (next != NULL)
    {
      unyte_option_t *cur = next;
      next = next->next;
      free(cur->data);
      free(cur);
    }
    free(current->header->options);
    free(current->header);
    current++;
  }
  free(packets->segments);
  free(packets);
  return 0;
}

int free_unyte_sent_message(unyte_message_t *msg)
{
  free(msg->options);
  free(msg);
  return 0;
}
