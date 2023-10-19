#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../src/hexdump.h"
#include "../src/unyte_udp_collector.h"
#include "../src/listening_worker.h"

#define USED_VLEN 1
#define MAX_TO_RECEIVE 10

int main(int argc, char *argv[])
{
  if (argc != 6)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./client_sample <ip> <port> <ca-cert> <server-cert> <server-key>\n");
    exit(1);
  }

  // Create a udp socket with default socket buffer
  //int socketfd = unyte_udp_create_socket(argv[1], argv[2], DEFAULT_SK_BUFF_SIZE);

  printf("Listening on %s:%s\n", argv[1], argv[2]);
  
  int sockfd = dtls_server_launcher(argv[1], atoi(argv[2]), argv[3], argv[4], argv[5]);

  printf("socket fd = %d\n", sockfd);
  
  // Initialize collector options
  // unyte_udp_options_t options = {0};
  // options.recvmmsg_vlen = USED_VLEN;
  // options.socket_fd = socketfd; // passing socket file descriptor to listen to
  // options.msg_dst_ip = false;   // destination IP not parsed from IP packet to improve performance
  // options.nb_parsers = 1;

  // /* Initialize collector */
  // unyte_udp_collector_t *collector = unyte_udp_start_collector(&options);
  // int recv_count = 0;
  // int max = MAX_TO_RECEIVE;

  // while (recv_count < max)
  // {
  //   /* Read queue */
  //   void *seg_pointer = unyte_udp_queue_read(collector->queue);
  //   if (seg_pointer == NULL)
  //   {
  //     printf("seg_pointer null\n");
  //     fflush(stdout);
  //   }
  //   unyte_seg_met_t *seg = (unyte_seg_met_t *)seg_pointer;

  //   // printf("unyte_udp_get_version: %u\n", unyte_udp_get_version(seg));
  //   // printf("unyte_udp_get_space: %u\n", unyte_udp_get_space(seg));
  //   // printf("unyte_udp_get_media_type: %u\n", unyte_udp_get_media_type(seg));
  //   printf("ok client sample\n");
  //   printf("unyte_udp_get_header_length: %u\n", unyte_udp_get_header_length(seg));
  //   printf("unyte_udp_get_message_length: %u\n", unyte_udp_get_message_length(seg));
  //   // printf("unyte_udp_get_observation_domain_id: %u\n", unyte_udp_get_observation_domain_id(seg));
  //   // printf("unyte_udp_get_message_id: %u\n", unyte_udp_get_message_id(seg));
  //   // printf("unyte_udp_get_src[family]: %u\n", unyte_udp_get_src(seg)->ss_family);
  //   // printf("unyte_udp_get_dest_addr[family]: %u\n", unyte_udp_get_dest_addr(seg) == NULL ? 0 : unyte_udp_get_dest_addr(seg)->ss_family); // NULL if options.msg_dst_ip is set to false (default)
  //   // char ip_canonical[100];
  //   // if (unyte_udp_get_src(seg)->ss_family == AF_INET) {
  //   //   printf("src IPv4: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_addr.s_addr, ip_canonical, sizeof ip_canonical));
  //   //   printf("src port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_src(seg))->sin_port));
  //   // } else {
  //   //   printf("src IPv6: %s\n", inet_ntop(unyte_udp_get_src(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_addr.s6_addr, ip_canonical, sizeof ip_canonical));
  //   //   printf("src port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_src(seg))->sin6_port));
  //   // }
  //   // Only if options.msg_dst_ip is set to true
  //   // printf("ok client sample\n");
  //   if (unyte_udp_get_dest_addr(seg) != NULL) {
  //     char ip_dest_canonical[100];
  //     if (unyte_udp_get_dest_addr(seg)->ss_family == AF_INET) {
  //       printf("dest IPv4: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_addr.s_addr, ip_dest_canonical, sizeof ip_dest_canonical));
  //       printf("dest port: %u\n", ntohs(((struct sockaddr_in*)unyte_udp_get_dest_addr(seg))->sin_port));
  //     } else {
  //       printf("dest IPv6: %s\n", inet_ntop(unyte_udp_get_dest_addr(seg)->ss_family, &((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_addr.s6_addr, ip_dest_canonical, sizeof ip_dest_canonical));
  //       printf("dest port: %u\n", ntohs(((struct sockaddr_in6*)unyte_udp_get_dest_addr(seg))->sin6_port));
  //     }
  //   }
  //   // printf("unyte_udp_get_payload: %.*s\n", unyte_udp_get_payload_length(seg), unyte_udp_get_payload(seg)); // payload may not be NULL-terminated
  //   // printf("unyte_udp_get_payload_length: %u\n", unyte_udp_get_payload_length(seg));

  //   /* Processing sample */
  //   recv_count++;
  //   print_udp_notif_header(seg->header, stdout);
  //   hexdump(seg->payload, seg->header->message_length - seg->header->header_length);
 
  //   fflush(stdout);

  //   /* Struct frees */
  //   unyte_udp_free_all(seg);
  // }

  // printf("Shutdown the socket\n");
  // close(*collector->sockfd);
  // pthread_join(*collector->main_thread, NULL);

  // // freeing collector mallocs
  // unyte_udp_free_collector(collector);
  // fflush(stdout);
  return 0;
}