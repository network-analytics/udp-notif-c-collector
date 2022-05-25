#include <stdio.h>
#include <stdlib.h>

#include "../src/unyte_sender.h"
#include "../src/unyte_udp_utils.h"
#include "../src/unyte_udp_constants.h"

#define MTU 1500

struct buffer_to_send
{
  char *buffer;
  uint buffer_len;
};

struct buffer_to_send *read_cbor_file()
{
  uint bytes = 754;
  struct buffer_to_send *bf = (struct buffer_to_send *)malloc(sizeof(struct buffer_to_send));
  bf->buffer = (char *)malloc(bytes);
  bf->buffer_len = bytes;
  FILE *fptr;
  if ((fptr = fopen("resources/cbor-example.cbor", "rb")) == NULL)
  {
    printf("Error! opening file");
    // Program exits if the file pointer returns NULL.
    exit(1);
  }
  fread(bf->buffer, bytes, 1, fptr);
  fclose(fptr);
  return bf;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./sender_cbor <ip> <port>\n");
    exit(1);
  }

  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = argv[1];
  options.port = argv[2];
  options.default_mtu = MTU;
  printf("Init sender on %s:%s | mtu: %d\n", options.address, options.port, MTU);

  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);

  struct buffer_to_send *bf_send = read_cbor_file(); // resources/cbor-exampole.cbor: 754 bytes

  unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
  message->buffer = bf_send->buffer;
  message->buffer_len = bf_send->buffer_len;
  // UDP-notif
  message->version = 0;
  message->space = UNYTE_SPACE_STANDARD;
  message->encoding_type = UNYTE_ENCODING_CBOR; // sending CBOR
  message->observation_domain_id = 1000;
  message->message_id = 2147483669;
  message->used_mtu = 0;   // use default configured
  message->options = NULL; // no custom options sent
  message->options_len = 0;

  unyte_send(sender_sk, message);

  // Freeing message and socket
  free(message);
  free(bf_send->buffer);
  free(bf_send);
  free_sender_socket(sender_sk);
  return 0;
}
