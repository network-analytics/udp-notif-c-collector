#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/unyte_sender.h"
#include "../src/unyte_utils.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define MTU 1500
#define LAST_GEN_ID 49993648

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

int main(int argc, char *argv[])
{
  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.default_mtu = MTU;

  uint messages_to_send = 10000;
  uint gen_id = 0;
  uint msg_id = 0;

  if (argc == 5)
  { // Usage: ./sender_performance <address> <port> <message_to_send> <gen_id>
    options.address = argv[1];
    options.port = atoi(argv[2]);
    messages_to_send = atoi(argv[3]);
    gen_id = atoi(argv[4]);
  }

  printf("Init sender on %s:%d|%d|sending:%d|gid:%d\n", options.address, options.port, MTU, messages_to_send, gen_id);
  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);

  struct buffer_to_send *bf_send = big_json_file();
  // struct buffer_to_send *bf_send = small_json_file();
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
    message->used_mtu = 0; // use default configured

    unyte_send(sender_sk, message);

    // To slow down the sender
    // int sleep_mod = 10000;
    // if (messages_to_send % sleep_mod == 0) {
    //   nanosleep(&t, NULL);
    //   printf("Sent %d\n", sleep_mod);
    //   fflush(stdout);
    // }

    if (gen_id != LAST_GEN_ID)
    {
      gen_id++;
    }
  }

  // Freeing message and socket
  free(message);
  free(bf_send->buffer);
  free(bf_send);
  free_sender_socket(sender_sk);
  return 0;
}
