#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "../src/hexdump.h"
#include "../src/unyte_collector.h"
#include "../src/unyte_utils.h"
#include "../src/queue.h"

#define USED_VLEN 10
#define MAX_TO_RECEIVE 200

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./client_sample <ip> <port>\n");
    exit(1);
  }

  // Initialize collector options
  unyte_options_t options = {0};
  options.recvmmsg_vlen = USED_VLEN;
  options.address = argv[1];
  options.port = atoi(argv[2]);
  printf("Listening on %s:%d\n", options.address, options.port);

  /* Initialize collector */
  unyte_collector_t *collector = unyte_start_collector(&options);
  int recv_count = 0;
  int max = MAX_TO_RECEIVE;

  while (recv_count < max)
  {
    /* Read queue */
    void *seg_pointer = unyte_queue_read(collector->queue);
    if (seg_pointer == NULL)
    {
      printf("seg_pointer null\n");
      fflush(stdout);
    }
    unyte_seg_met_t *seg = (unyte_seg_met_t *)seg_pointer;

    // printf("get_version: %u\n", get_version(seg));
    // printf("get_space: %u\n", get_space(seg));
    // printf("get_encoding_type: %u\n", get_encoding_type(seg));
    // printf("get_header_length: %u\n", get_header_length(seg));
    // printf("get_message_length: %u\n", get_message_length(seg));
    // printf("get_generator_id: %u\n", get_generator_id(seg));
    // printf("get_message_id: %u\n", get_message_id(seg));
    // printf("get_src_port: %u\n", get_src_port(seg));
    // printf("get_src_addr: %u\n", get_src_addr(seg));
    // printf("get_dest_addr: %u\n", get_dest_addr(seg));
    // printf("get_payload: %s\n", get_payload(seg));
    // printf("get_payload_length: %u\n", get_payload_length(seg));

    /* Processing sample */
    recv_count++;
    printHeader(seg->header, stdout);
    // hexdump(seg->payload, seg->header->message_length - seg->header->header_length);

    fflush(stdout);

    /* Struct frees */
    unyte_free_all(seg);
  }

  printf("Shutdown the socket\n");
  close(*collector->sockfd);
  pthread_join(*collector->main_thread, NULL);

  // freeing collector mallocs
  unyte_free_collector(collector);
  fflush(stdout);
  return 0;
}
