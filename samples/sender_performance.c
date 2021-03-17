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

struct sleep_msg {
  uint msg_mod;
  uint milisec;
};

struct sender_th_input
{
  unyte_sender_options_t *sender_opt;
  uint used_mtu;
  uint msg_size;
  uint message_to_send;
  int msg_type; // 0 = gen_id++ | 1 = msg_id++
  uint init_gid;
  uint init_mid;
  struct sleep_msg *sleep_messages;
};

void *t_send(void *in)
{
  struct sender_th_input *input = (struct sender_th_input *)in;

  pthread_t thread_id = pthread_self();
  struct unyte_sender_socket *sender_sk = unyte_start_sender(input->sender_opt);

  struct buffer_to_send *bf_send = read_json_file(input->msg_size);

  unyte_message_t *message = (unyte_message_t *)malloc(sizeof(unyte_message_t));
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = 999999 * input->sleep_messages->milisec; // 30 ms sleep
  uint messages = input->message_to_send;
  uint gen_id = input->init_gid;
  uint msg_id = input->init_mid;
  printf("Thread: %ld|%d|%d|%d|%d|%d|%d\n", thread_id, messages, gen_id, msg_id, input->msg_type, input->sleep_messages->milisec, input->sleep_messages->msg_mod);
  while (messages--)
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
    message->used_mtu = input->used_mtu; // use default configured if 0

    unyte_send(sender_sk, message);

    // To slow down the sender
    if (input->sleep_messages->milisec > 0) {
      if (messages % input->sleep_messages->msg_mod == 0) {
        nanosleep(&t, NULL);
        // printf("HERE\n");
        fflush(stdout);
      }
    }
    if (input->msg_type == 0)
    {
      if (gen_id != LAST_GEN_ID)
      {
        gen_id++;
      }
    }
    else
    {
      if (msg_id != LAST_GEN_ID)
      {
        msg_id++;
      }
    }
  }

  free(message);
  free(bf_send->buffer);
  free(bf_send);
  free_sender_socket(sender_sk);
  return 0;
}
struct sender_threads
{
  pthread_t *threads;
  struct sender_th_input *sender_inputs;
  uint count;
};

struct sender_threads *create_senders(uint count, struct sender_th_input *gl_sender_input)
{
  struct sender_threads *senders = (struct sender_threads *)malloc(sizeof(struct sender_threads));

  pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * count);
  struct sender_th_input *sender_inputs = (struct sender_th_input *)malloc(sizeof(struct sender_th_input) * count);
  pthread_t *cur = threads;
  struct sender_th_input *cur_input = sender_inputs;
  uint msg_per_th = gl_sender_input->message_to_send / count;
  for (uint i = 0; i < count; i++)
  {
    cur_input->msg_size = gl_sender_input->msg_size;
    cur_input->msg_type = gl_sender_input->msg_type;
    cur_input->sender_opt = gl_sender_input->sender_opt;
    cur_input->used_mtu = gl_sender_input->used_mtu;
    // variable
    if (gl_sender_input->init_gid == LAST_GEN_ID)
      cur_input->init_gid = LAST_GEN_ID;
    else
      cur_input->init_gid = msg_per_th * i;
    if (gl_sender_input->init_mid == LAST_GEN_ID)
      cur_input->init_mid = LAST_GEN_ID;
    else
      cur_input->init_mid = msg_per_th * i;
    cur_input->message_to_send = msg_per_th;
    cur_input->sleep_messages = gl_sender_input->sleep_messages;
    pthread_create(cur, NULL, t_send, (void *)cur_input);
    cur++;
    cur_input++;
  }
  senders->count = count;
  senders->threads = threads;
  senders->sender_inputs = sender_inputs;
  return senders;
}

void join_senders(struct sender_threads *senders)
{
  pthread_t *cur = senders->threads;
  for (uint i = 0; i < senders->count; i++)
  {
    pthread_join(*cur, NULL);
    cur++;
  }
}

void clean_sender_threads(struct sender_threads *threads)
{
  free(threads->threads);
  free(threads->sender_inputs);
  free(threads);
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
  uint threads_to_start = 1;
  struct sleep_msg sleep = {0};
  if (argc == 11)
  { // Usage: ./sender_performance <address> <port> <message_to_send> <gen_id> <type_msg> <mtu> <msg_bytes_size> <threads> <milisec_sleep> <messages_mod>
    options.address = argv[1];
    options.port = atoi(argv[2]);
    messages_to_send = atoi(argv[3]);
    gen_id = atoi(argv[4]);
    msg_type = atoi(argv[5]);
    if (msg_type == 1)
    {
      msg_id = atoi(argv[4]);
      gen_id = 0;
    }
    used_mtu = atoi(argv[6]);
    msg_size = atoi(argv[7]);
    options.default_mtu = used_mtu;
    threads_to_start = atoi(argv[8]);
    uint sleep_milis = atoi(argv[9]);
    if (sleep_milis > 0) {
      sleep.milisec = sleep_milis;
      sleep.msg_mod = atoi(argv[10]);
    }
  } else {
    printf("Using defaults\n");
  }

  printf("Init sender on %s:%d|%d|sending:%d|gid:%d|size:%d\n", options.address, options.port, options.default_mtu, messages_to_send, gen_id, msg_size);
  struct sender_th_input sender_input = {0};
  sender_input.msg_size = msg_size;
  sender_input.msg_type = msg_type;
  sender_input.sender_opt = &options;
  sender_input.used_mtu = used_mtu;
  sender_input.init_gid = gen_id;
  sender_input.init_mid = msg_id;
  sender_input.message_to_send = messages_to_send;
  sender_input.sleep_messages = &sleep;
  struct sender_threads *threads = create_senders(threads_to_start, &sender_input);
  join_senders(threads);
  clean_sender_threads(threads);

  return 0;
}
