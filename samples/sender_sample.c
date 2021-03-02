#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/unyte_sender.h"
#include "../src/unyte_utils.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define MTU 1500

int main()
{
  printf("Init sender on %s:%d|%d\n", ADDR, PORT, MTU);
  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.default_mtu = MTU;

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
  unyte_send(sender_sk, message);

  // Freeing message and socket
  free(message);
  free_sender_socket(sender_sk);
  return 0;
}
