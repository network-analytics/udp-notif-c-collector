#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "monitoring_worker.h"

unyte_seg_counters_t *init_counters(uint nb_threads)
{
  unyte_seg_counters_t *counters = (unyte_seg_counters_t *)malloc(sizeof(unyte_seg_counters_t) * (nb_threads));
  unyte_seg_counters_t *cur = counters;
  for (uint i = 0; i < nb_threads; i++)
  {
    reinit_counters(cur);
    cur++;
  }
  return counters;
}

void update_lost_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid)
{
  counters->last_generator_id = last_gid;
  counters->last_message_id = last_mid;
  counters->segments_lost++;
}

void update_reordered_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid)
{
  counters->last_generator_id = last_gid;
  counters->last_message_id = last_mid;
  counters->segments_reordered++;
}

void update_ok_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid)
{
  counters->last_generator_id = last_gid;
  counters->last_message_id = last_mid;
  counters->segments_count++;
}

void reinit_counters(unyte_seg_counters_t *counters)
{
  counters->last_generator_id = 0;
  counters->last_message_id = 0;
  counters->segments_count = 0;
  counters->segments_lost = 0;
  counters->segments_reordered = 0;
}

unyte_seg_counters_t *clone_counters(unyte_seg_counters_t *src)
{
  unyte_seg_counters_t *counters = (unyte_seg_counters_t *)malloc(sizeof(unyte_seg_counters_t));
  memcpy(counters, src, sizeof(unyte_seg_counters_t));
  return counters;
}

void print_counters(unyte_seg_counters_t *counter, FILE *std)
{
  fprintf(std, "\n*** Counters ***\n");
  fprintf(std, "Thread: %lu\n", counter->thread_id);
  fprintf(std, "Type: %s\n", counter->type == PARSER_WORKER ? "PARSER_WORKER" : "LISTENER_WORKER");
  fprintf(std, "OK: %u\n", counter->segments_count);
  fprintf(std, "Lost: %u\n", counter->segments_lost);
  fprintf(std, "Reorder: %u\n", counter->segments_reordered);
  fprintf(std, "Last gen id: %u\n", counter->last_generator_id);
  fprintf(std, "Last msg id: %u\n", counter->last_message_id);
  fprintf(std, "***** End ******\n");
}

void *t_monitoring(void *in)
{
  struct monitoring_thread_input *input = (struct monitoring_thread_input *)in;
  queue_t *output_queue = input->output_queue;
  while (1)
  {
    sleep(input->delay);
    unyte_seg_counters_t *cur = input->counters;
    for (uint i = 0; i < input->nb_counters; i++)
    {
      unyte_seg_counters_t *clone = clone_counters(cur);
      unyte_queue_destructive_write(output_queue, clone);
      reinit_counters(cur);
      cur++;
    }
  }
  return NULL;
}

pthread_t get_thread_id(unyte_seg_counters_t *counter) { return counter->thread_id; }
uint32_t get_last_gen_id(unyte_seg_counters_t *counter) { return counter->last_generator_id; }
uint32_t get_last_msg_id(unyte_seg_counters_t *counter) { return counter->last_message_id; }
uint32_t get_ok_seg(unyte_seg_counters_t *counter) { return counter->segments_count; }
uint32_t get_lost_seg(unyte_seg_counters_t *counter) { return counter->segments_lost; }
uint32_t get_reordered_seg(unyte_seg_counters_t *counter) { return counter->segments_reordered; }
thread_type_t get_th_type(unyte_seg_counters_t *counter) { return counter->type; }
