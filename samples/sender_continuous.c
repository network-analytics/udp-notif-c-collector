#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/unyte_sender.h"
#include "../src/unyte_utils.h"

#define PORT 8081
#define ADDR "192.168.0.17"
#define MTU 1500
#define GEN_ID 232
#define MSG_SIZE 8950
#define SLEEP_MS 300
#define SLEEP_MSG 20
#define SLEEP_JITTER 0.3
#define INTERFACE ""

struct buffer_to_send
{
  char *buffer;
  uint buffer_len;
};

struct sleep_msg
{
  uint msg_mod;
  uint milisec;
  float jitter;
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
  // Initialize collector options
  unyte_sender_options_t options = {0};
  options.address = ADDR;
  options.port = PORT;
  options.default_mtu = MTU;
  options.interface = INTERFACE;

  uint gen_id = GEN_ID;
  uint used_mtu = 0;
  uint msg_size = MSG_SIZE;
  struct sleep_msg sleep = {0};
  sleep.milisec = SLEEP_MS;
  sleep.msg_mod = SLEEP_MSG;
  sleep.jitter = SLEEP_JITTER;

  if (argc == 10 || argc == 9)
  { // Usage: ./sender_continuous <address> <port> <gen_id> <mtu> <msg_bytes_size> <milisec_sleep> <messages_mod> <jitter> <interface>
    options.address = argv[1];
    options.port = atoi(argv[2]);
    gen_id = atoi(argv[3]);
    used_mtu = atoi(argv[4]);
    msg_size = atoi(argv[5]);
    options.default_mtu = used_mtu;
    sleep.milisec = atoi(argv[6]);
    sleep.msg_mod = atoi(argv[7]);
    sleep.jitter = atof(argv[8]);
    if (argc == 10) {
      options.interface = argv[9];
    } else {
      options.interface = "";
    }
  }
  else
  {
    printf("Using defaults\n");
  }
  pid_t pid = getpid();

  printf("Sender %d;%s:%d|%d|gid:%d|size:%d|sleep:%d|sleep_mod:%d|jitter:%f\n", pid, options.address, options.port, options.default_mtu, gen_id, msg_size, sleep.milisec, sleep.msg_mod, sleep.jitter);

  struct unyte_sender_socket *sender_sk = unyte_start_sender(&options);

  struct buffer_to_send *bf_send = read_json_file(msg_size);

  unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
  uint msg_id = 0;

  struct timespec t;
  t.tv_sec = 0;

  time_t t_seed;
  srand((unsigned) time(&t_seed));
  uint sleep_nanosec = 999999 * sleep.milisec;
  uint lower_sleep = sleep_nanosec * (1 - sleep.jitter);
  uint upper_sleep = sleep_nanosec * (1 + sleep.jitter);
  while (1)
  {
    message->buffer = bf_send->buffer;
    message->buffer_len = bf_send->buffer_len;
    // UDP-notif
    message->version = 0;
    message->space = 0;
    message->encoding_type = 1;
    message->generator_id = gen_id;
    message->message_id = msg_id;
    message->used_mtu = 0; // use default configured

    unyte_send(sender_sk, message);
    msg_id++;

    // To slow down the sender
    if (sleep.milisec > 0) {
      if (msg_id % sleep.msg_mod == 0) {
        t.tv_nsec = (rand() % (upper_sleep - lower_sleep + 1)) + lower_sleep;
        // printf("%d: t.tv_nsec: %ld\n", pid, t.tv_nsec/999999);
        nanosleep(&t, NULL);
        fflush(stdout);
      }
    }
  }
  return 0;
}
