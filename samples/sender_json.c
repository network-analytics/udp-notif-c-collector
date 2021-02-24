#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/unyte_sender.h"
#include "../src/unyte_utils.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define MTU 1500

struct buffer_to_send
{
  char *buffer;
  uint buffer_len;
};

struct buffer_to_send *small_json_file()
{
  struct buffer_to_send *bf = (struct buffer_to_send *)malloc(sizeof(struct buffer_to_send));
  bf->buffer = (char *)malloc(716);
  bf->buffer_len = 716;
  FILE *fptr;
  if ((fptr = fopen("resources/small.json", "r")) == NULL)
  {
    printf("Error! opening file");
    // Program exits if the file pointer returns NULL.
    exit(1);
  }
  fread(bf->buffer, 716, 1, fptr);
  fclose(fptr);
  return bf;
}

struct buffer_to_send *big_json_file()
{
  struct buffer_to_send *bf = (struct buffer_to_send *)malloc(sizeof(struct buffer_to_send));
  bf->buffer = (char *)malloc(7151);
  bf->buffer_len = 7151;
  FILE *fptr;
  if ((fptr = fopen("resources/big.json", "r")) == NULL)
  {
    printf("Error! opening file");
    // Program exits if the file pointer returns NULL.
    exit(1);
  }
  fread(bf->buffer, 7151, 1, fptr);
  fclose(fptr);
  return bf;
}

int main()
{
  printf("Init sender on %s:%d|%d\n", ADDR, PORT, MTU);
  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.default_mtu = MTU;

  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);

  struct buffer_to_send *bf_send = big_json_file();
  unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
  message->buffer = bf_send->buffer;
  message->buffer_len = bf_send->buffer_len;
  // UDP-notif
  message->version = 0;
  message->space = 0;
  message->encoding_type = 1;
  message->generator_id = 1000;
  message->message_id = 2147483669;
  message->used_mtu = 0; // use default configured

  unyte_send(sender_sk, message);

  // Freeing message and socket
  free(message);
  free(bf_send->buffer);
  free(bf_send);
  free_sender_socket(sender_sk);
  return 0;
}
