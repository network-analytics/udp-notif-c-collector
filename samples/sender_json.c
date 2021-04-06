#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/unyte_sender.h"
#include "../src/unyte_utils.h"

#define PORT 10000
#define ADDR "192.168.0.17"
#define MTU 1500

struct buffer_to_send
{
  char *buffer;
  uint buffer_len;
};

struct buffer_to_send *read_json_file(uint bytes)
{
  struct buffer_to_send *bf = (struct buffer_to_send *)malloc(sizeof(struct buffer_to_send));
  bf->buffer = (char *)malloc(bytes);
  bf->buffer_len = bytes;
  FILE *fptr;
  int filename_size = 24;
  if (bytes > 999) {
    filename_size = 25;
  } 
  char *filename = (char *)malloc(filename_size);
  snprintf(filename, filename_size, "resources/json-%d.json", bytes);
  if ((fptr = fopen(filename, "r")) == NULL)
  {
    printf("Error! opening file");
    free(filename);
    // Program exits if the file pointer returns NULL.
    exit(1);
  }
  free(filename);
  fread(bf->buffer, bytes, 1, fptr);
  fclose(fptr);
  return bf;
}

struct buffer_to_send *small_json_file()
{
  return read_json_file(700);
}

struct buffer_to_send *big_json_file()
{
  return read_json_file(8950);
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
  // struct buffer_to_send *bf_send = read_json_file(200);

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
