#include <stdio.h>
#include <stdlib.h>

#include "../src/unyte_sender.h"

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
  options.port = argv[2];
  options.default_mtu = MTU;
  printf("Init sender on %s:%s | mtu: %d\n", options.address, options.port, MTU);

  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);
  int reuse = 0;

  for(int i = 0; i < 5; i++){
      if(i != 0){
          reuse = 1;
      }

      char string_to_send[50];
      sprintf((char *) string_to_send, "%s %d", "Message", i);
      printf("MESSAGE 2 = %s\n", string_to_send);

      unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));

      message->buffer = string_to_send;
      message->buffer_len = strlen(string_to_send);
      // UDP-notif
      message->version = 0;
      message->space = 0;
      message->media_type = 1; // json but sending string
      message->observation_domain_id = 1000;
      message->message_id = 2147483669;
      message->used_mtu = 200; // use other than default configured
      message->options = NULL;
      message->options_len = 0; // should be initialized to 0 if no options are wanted

      printf("RETURNED VALUE = %d\n", unyte_send_with_dtls_context(sender_sk, message, reuse));
      printf("SORTIE\n");
      free(message);
  }
  printf("avant free\n");
  free_sender_socket(sender_sk);
  printf("apres sortie\n");
  return 0;
}
