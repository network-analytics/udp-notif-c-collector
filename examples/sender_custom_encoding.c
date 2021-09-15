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

struct buffer_to_send *read_json_file(uint bytes)
{
  struct buffer_to_send *bf = (struct buffer_to_send *)malloc(sizeof(struct buffer_to_send));
  bf->buffer = (char *)malloc(bytes);
  bf->buffer_len = bytes;
  FILE *fptr;
  int filename_size = 24;
  if (bytes > 999)
  {
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

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Error: arguments not valid\n");
    printf("Usage: ./sender_custom_encoding <ip> <port>\n");
    exit(1);
  }

  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = argv[1];
  options.port = atoi(argv[2]);
  options.default_mtu = MTU;
  printf("Init sender on %s:%d | mtu: %d\n", options.address, options.port, MTU);

  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);

  struct buffer_to_send *bf_send = read_json_file(200); // resources/json-200.json
  // struct buffer_to_send *bf_send = read_json_file(8950); // resources/json-8950.json

  unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
  message->buffer = bf_send->buffer;
  message->buffer_len = bf_send->buffer_len;
  // UDP-notif
  message->version = 0;
  message->space = UNYTE_SPACE_NON_STANDARD; // Custom space
  message->encoding_type = 4;                // Custom encoding type
  message->generator_id = 1000;
  message->message_id = 2147483669;
  message->used_mtu = 0; // use default configured

  unyte_send_option_t *udp_options = (unyte_send_option_t *)malloc(sizeof(unyte_send_option_t) * 2);

  udp_options[0].type = UNYTE_TYPE_PRIVATE_ENCODING; // private encoding
  udp_options[0].data = "CSV";
  udp_options[0].data_length = 3;

  udp_options[1].type = 3; // custom header
  udp_options[1].data = "Custom";
  udp_options[1].data_length = 6;

  message->options = udp_options;
  message->options_len = 2;

  unyte_send(sender_sk, message);

  // Freeing message and socket
  free_unyte_sent_message(message);
  free(bf_send->buffer);
  free(bf_send);
  free_sender_socket(sender_sk);
  return 0;
}
