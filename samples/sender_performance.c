#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/unyte_sender.h"
#include "../src/unyte_utils.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define MTU 1500
#define LAST_GEN_ID 49993648
#define MAX_TO_SEND 1000

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
  char *filename;
  int filename_size = 24;
  if (bytes > 999) {
    filename_size = 25;
  } 
  filename = (char *)malloc(filename_size); 
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
  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.default_mtu = MTU;

  uint messages_to_send = MAX_TO_SEND;
  uint gen_id = 0;
  uint msg_id = 0;
  int msg_type = 0; // 0 = gen_id++ | 1 = msg_id++
  uint used_mtu = 0;
  uint msg_size = 8950;

  if (argc == 8)
  { // Usage: ./sender_performance <address> <port> <message_to_send> <gen_id> <type_msg> <mtu> <msg_bytes_size>
    options.address = argv[1];
    options.port = atoi(argv[2]);
    messages_to_send = atoi(argv[3]);
    gen_id = atoi(argv[4]);
    msg_type = atoi(argv[5]);
    if (msg_type == 1) {
      msg_id = atoi(argv[4]);
      gen_id = 0;
    }
    used_mtu = atoi(argv[6]);
    msg_size = atoi(argv[7]);
    options.default_mtu = used_mtu;
  }

  printf("Init sender on %s:%d|%d|sending:%d|gid:%d|size:%d\n", options.address, options.port, options.default_mtu, messages_to_send, gen_id, msg_size);
  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);

  struct buffer_to_send *bf_send = read_json_file(msg_size);

  unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
  // struct timespec t;
  // t.tv_sec = 0;
  // t.tv_nsec = 999999 * 100; // 100 ms sleep

  while (messages_to_send--)
  {
    message->buffer = bf_send->buffer;
    message->buffer_len = bf_send->buffer_len;
    // message->buffer = "A";
    // message->buffer_len = 1;

    // UDP-notif
    message->version = 0;
    message->space = 0;
    message->encoding_type = 1;
    message->generator_id = gen_id;
    message->message_id = msg_id;
    message->used_mtu = used_mtu; // use default configured if 0

    unyte_send(sender_sk, message);

    // To slow down the sender
    // int sleep_mod = 10000;
    // if (messages_to_send % sleep_mod == 0) {
    //   nanosleep(&t, NULL);
    //   printf("Sent %d\n", sleep_mod);
    //   fflush(stdout);
    // }
    if (msg_type == 0) {
      if (gen_id != LAST_GEN_ID)
      {
        gen_id++;
      }
    } else {
      if (msg_id != LAST_GEN_ID)
      {
        msg_id++;
      }
    }
  }

  // Freeing message and socket
  free(message);
  free(bf_send->buffer);
  free(bf_send);
  free_sender_socket(sender_sk);
  return 0;
}
