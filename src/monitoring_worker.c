#include <unistd.h>
#include <pthread.h>
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
  counters->last_generator_id = src->last_generator_id;
  counters->last_message_id = src->last_message_id;
  counters->segments_count = src->segments_count;
  counters->segments_lost = src->segments_lost;
  counters->segments_reordered = src->segments_reordered;
  counters->thread_id = src->thread_id;
  return counters;
}

void print_counters(unyte_seg_counters_t *counter, FILE *std)
{
  fprintf(std, "\n*** Counters ***\n");
  fprintf(std, "Thread: %lu\n", counter->thread_id);
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
