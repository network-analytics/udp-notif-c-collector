#include <stdio.h>
#include <stdlib.h>

#include "../src/unyte_sender.h"
#include "../src/unyte_udp_utils.h"

#define MTU 1500

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./sender_sample <ip> <port>\n");
    exit(1);
  }

  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = argv[1];
  options.port = atoi(argv[2]);
  options.default_mtu = MTU;
  printf("Init sender on %s:%d | mtu: %d\n", options.address, options.port, MTU);

  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);

  char *string_to_send = "Hello world1! Hello world2! Hello world3! Hello world4! Hello world5! Hello world6! Hello world7!";
  unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
  message->buffer = string_to_send;
  message->buffer_len = 97;
  // UDP-notif
  message->version = 0;
  message->space = 0;
  message->encoding_type = 1; // json but sending string
  message->generator_id = 1000;
  message->message_id = 2147483669;
  message->used_mtu = 200; // use other than default configured
  message->options = NULL;
  message->options_len = 0; // should be initialized to 0 if no options are wanted

  unyte_send(sender_sk, message);

  // Freeing message and socket
  free(message);
  free_sender_socket(sender_sk);
  return 0;
}
