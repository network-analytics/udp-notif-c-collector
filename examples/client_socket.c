#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../src/hexdump.h"
#include "../src/unyte_udp_collector.h"
#include "../src/unyte_udp_utils.h"
#include "../src/unyte_udp_queue.h"
#include "../src/unyte_udp_defaults.h"

#define USED_VLEN 1
#define MAX_TO_RECEIVE 20

/**
 * Creates own custom socket
 */
int create_custom_socket(char *address, char *port)
{
  struct addrinfo *addr_info;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

  // Using getaddrinfo to support both IPv4 and IPv6
  int rc = getaddrinfo(address, port, &hints, &addr_info);

  if (rc != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(rc));
    exit(EXIT_FAILURE);
  }

  printf("Address type: %s | %d\n", (addr_info->ai_family == AF_INET) ? "IPv4" : "IPv6", ntohs(((struct sockaddr_in *)addr_info->ai_addr)->sin_port));

  // create socket on UDP protocol
  int sockfd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);

  // handle error
  if (sockfd < 0)
  {
    perror("Cannot create socket");
    exit(EXIT_FAILURE);
  }

  // Use SO_REUSEPORT to be able to launch multiple collector on the same address
  int optval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(int)) < 0)
  {
    perror("Cannot set SO_REUSEPORT option on socket");
    exit(EXIT_FAILURE);
  }

  // Setting socket buffer to default 20 MB
  uint64_t receive_buf_size = DEFAULT_SK_BUFF_SIZE;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &receive_buf_size, sizeof(receive_buf_size)) < 0)
  {
    perror("Cannot set buffer size");
    exit(EXIT_FAILURE);
  }

  if (bind(sockfd, addr_info->ai_addr, (int)addr_info->ai_addrlen) == -1)
  {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // free addr_info after usage
  freeaddrinfo(addr_info);

  return sockfd;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./client_socket <ip> <port>\n");
    exit(1);
  }

  int sockfd = create_custom_socket(argv[1], argv[2]);

  // Initialize collector options
  unyte_udp_options_t options = {0};
  options.recvmmsg_vlen = USED_VLEN;
  options.socket_fd = sockfd;
  printf("Listening on socket %d\n", options.socket_fd);

  /* Initialize collector */
  unyte_udp_collector_t *collector = unyte_udp_start_collector(&options);
  int recv_count = 0;
  int max = MAX_TO_RECEIVE;

  while (recv_count < max)
  {
    /* Read queue */
    void *seg_pointer = unyte_udp_queue_read(collector->queue);
    if (seg_pointer == NULL)
    {
      printf("seg_pointer null\n");
      fflush(stdout);
    }
    unyte_seg_met_t *seg = (unyte_seg_met_t *)seg_pointer;

    // printf("unyte_udp_get_version: %u\n", unyte_udp_get_version(seg));
    // printf("unyte_udp_get_space: %u\n", unyte_udp_get_space(seg));
    // printf("unyte_udp_get_media_type: %u\n", unyte_udp_get_media_type(seg));
    // printf("unyte_udp_get_header_length: %u\n", unyte_udp_get_header_length(seg));
    // printf("unyte_udp_get_message_length: %u\n", unyte_udp_get_message_length(seg));
    // printf("unyte_udp_get_observation_domain_id: %u\n", unyte_udp_get_observation_domain_id(seg));
    // printf("unyte_udp_get_message_id: %u\n", unyte_udp_get_message_id(seg));
    // printf("unyte_udp_get_src[family]: %u\n", unyte_udp_get_src(seg)->ss_family);
    // printf("unyte_udp_get_dest_addr[family]: %u\n", unyte_udp_get_dest_addr(seg)->ss_family);
    char ip_canonical[100];
    if (unyte_udp_get_src(seg)->ss_family == AF_INET) {
      printf("src IPv4: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_addr.s_addr, ip_canonical, sizeof ip_canonical));
      printf("src port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_port));
    } else {
      printf("src IPv6: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_addr.s6_addr, ip_canonical, sizeof ip_canonical));
      printf("src port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_port));
    }
    // Only if options.msg_dst_ip is set to true
    if (unyte_udp_get_dest_addr(seg) != NULL) {
      char ip_dest_canonical[100];
      if (unyte_udp_get_dest_addr(seg)->ss_family == AF_INET) {
        printf("dest IPv4: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_addr.s_addr, ip_dest_canonical, sizeof ip_dest_canonical));
        printf("dest port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_port));
      } else {
        printf("dest IPv6: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_addr.s6_addr, ip_dest_canonical, sizeof ip_dest_canonical));
        printf("dest port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_port));
      }
    }
    // printf("unyte_udp_get_payload: %s\n", unyte_udp_get_payload(seg));
    // printf("unyte_udp_get_payload_length: %u\n", unyte_udp_get_payload_length(seg));

    /* Processing sample */
    recv_count++;
    print_udp_notif_header(seg->header, stdout);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);

    fflush(stdout);

    /* Struct frees */
    unyte_udp_free_all(seg);
  }

  printf("Shutdown the socket\n");
  close(*collector->sockfd);
  pthread_join(*collector->main_thread, NULL);

  // freeing collector mallocs
  unyte_udp_free_collector(collector);
  fflush(stdout);
  return 0;
}
