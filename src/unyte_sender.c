#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <net/if.h>
#include <netdb.h>
#include "unyte_sender.h"

struct unyte_sender_socket *unyte_start_sender(unyte_sender_options_t *options)
{
  struct addrinfo *addr_info;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

  int rc = getaddrinfo(options->address, options->port, &hints, &addr_info);

  if (rc != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(rc));
    exit(EXIT_FAILURE);
  }

  struct unyte_sender_socket *sender_sk = (struct unyte_sender_socket *)malloc(sizeof(struct unyte_sender_socket));
  struct sockaddr_storage *address = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));

  if (address == NULL || sender_sk == NULL)
  {
    perror("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  // save copy of the connected addr
  memset(address, 0, sizeof(*address));
  memcpy(address, addr_info->ai_addr, addr_info->ai_addrlen);

  int sockfd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);

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

  // connect socket to destination address
  if (connect(sockfd, addr_info->ai_addr, (int)addr_info->ai_addrlen) == -1)
  {
    perror("Connect failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // free addr_info after usage
  freeaddrinfo(addr_info);

  sender_sk->sock_in = address;
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
