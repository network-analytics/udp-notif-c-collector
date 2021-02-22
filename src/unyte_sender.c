#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "unyte_sender.h"
#include "unyte_utils.h"

struct unyte_sender_socket *unyte_start_sender(unyte_sender_options_t *options)
{
  struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  addr->sin_family = AF_INET;
  addr->sin_port = htons(options->port);
  inet_pton(AF_INET, options->address, &addr->sin_addr);

  // if (bind(sockfd, (struct sockaddr *)addr, sizeof(*addr)) == -1)
  // {
  //   perror("Bind failed");
  //   close(sockfd);
  //   exit(EXIT_FAILURE);
  // }

  struct unyte_sender_socket *sender_sk = (struct unyte_sender_socket *)malloc(sizeof(struct unyte_sender_socket));
  sender_sk->sock_in = addr;
  sender_sk->sockfd = sockfd;
  sender_sk->default_mtu = options->default_mtu;
  return sender_sk;
}

int unyte_send(struct unyte_sender_socket *sender_sk, unyte_message_t *message)
{
  struct sockaddr_in *to = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
  to->sin_family = AF_INET;
  to->sin_port = htons(message->dest_port);
  inet_pton(AF_INET, message->dest_addr, &to->sin_addr);

  uint mtu = message->used_mtu;
  if (mtu <= 0)
  {
    if (sender_sk->default_mtu > 0)
      mtu = sender_sk->default_mtu;
    else
      mtu = DEFAULT_MTU;
  }

  struct unyte_segmented_msg *packets = build_message(message);
  unyte_seg_met_t *current_seg = packets->segments;
  for (uint i = 0; i < packets->segments_len; i++)
  {
    unsigned char *parsed_packet = serialize_message(current_seg);

    sendto(sender_sk->sockfd, parsed_packet, HEADER_BYTES + message->buffer_len, 0, (const struct sockaddr *)to, sizeof(*to));
    printf("(%d,%d) sent\n", current_seg->header->generator_id, current_seg->header->message_id);
    free(current_seg->header);
    free(current_seg);
    free(parsed_packet);
    current_seg++;
  }

  free(to);
  return 0;
}

int free_sender_socket(struct unyte_sender_socket *sender_sk)
{
  free(sender_sk->sock_in);
  free(sender_sk);
  return 0;
}
