#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "../src/hexdump.h"
#include "../src/unyte_udp_collector.h"
#include "../src/unyte_udp_utils.h"
#include "../src/unyte_udp_queue.h"

#define USED_VLEN 1
#define MAX_TO_RECEIVE 20

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./client_sample <ip> <port>\n");
    exit(1);
  }

  // Initialize collector options
  unyte_udp_options_t options = {0};
  options.recvmmsg_vlen = USED_VLEN;
  options.address = argv[1];
  options.port = atoi(argv[2]);
  printf("Listening on %s:%d\n", options.address, options.port);

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
    // printf("unyte_udp_get_encoding_type: %u\n", unyte_udp_get_encoding_type(seg));
    // printf("unyte_udp_get_header_length: %u\n", unyte_udp_get_header_length(seg));
    // printf("unyte_udp_get_message_length: %u\n", unyte_udp_get_message_length(seg));
    // printf("unyte_udp_get_generator_id: %u\n", unyte_udp_get_generator_id(seg));
    // printf("unyte_udp_get_message_id: %u\n", unyte_udp_get_message_id(seg));
    // printf("unyte_udp_get_src[family]: %u\n", unyte_udp_get_src(seg)->ss_family);
    // printf("unyte_udp_get_dest_addr[family]: %u\n", unyte_udp_get_dest_addr(seg)->ss_family);
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
