#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "unyte_sender.h"

struct unyte_sender_socket *unyte_start_sender(unyte_sender_options_t *options)
{
  struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
  struct unyte_sender_socket *sender_sk = (struct unyte_sender_socket *)malloc(sizeof(struct unyte_sender_socket));
  if (addr == NULL || sender_sk == NULL)
  {
    perror("Malloc failed.\n");
    exit(EXIT_FAILURE);
  }

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  addr->sin_family = AF_INET;
  addr->sin_port = htons(options->port);
  inet_pton(AF_INET, options->address, &addr->sin_addr);

  if (connect(sockfd, addr, sizeof(struct sockaddr_in)) == -1)
  {
    perror("Bind failed");
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
    send(sender_sk->sockfd, parsed_packet, current_seg->header->header_length + current_seg->header->message_length, 0);

    // printf("(%d,%d) sent\n", current_seg->header->generator_id, current_seg->header->message_id);
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
    free(current->header);
    current++;
  }
  free(packets->segments);
  free(packets);
  return 0;
}