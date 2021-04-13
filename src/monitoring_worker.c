#include <unistd.h>
#include <pthread.h>
#include "monitoring_worker.h"

void update_lost_segment(struct seg_counters *counters, uint32_t last_gid, uint32_t last_mid)
{
  counters->last_generator_id = last_gid;
  counters->last_message_id = last_mid;
  counters->segments_lost++;
}

void update_reordered_segment(struct seg_counters *counters, uint32_t last_gid, uint32_t last_mid)
{
  counters->last_generator_id = last_gid;
  counters->last_message_id = last_mid;
  counters->segments_reordered++;
}

void update_ok_segment(struct seg_counters *counters, uint32_t last_gid, uint32_t last_mid)
{
  counters->last_generator_id = last_gid;
  counters->last_message_id = last_mid;
  counters->segments_count++;
}

void reinit_counters(struct seg_counters *counters)
{
  counters->last_generator_id = 0;
  counters->last_message_id = 0;
  counters->segments_count = 0;
  counters->segments_lost = 0;
  counters->segments_reordered = 0;
}

struct seg_counters *clone_counters(struct seg_counters *src)
{
  struct seg_counters *counters = (struct seg_counters *)malloc(sizeof(struct seg_counters));
  counters->last_generator_id = src->last_generator_id;
  counters->last_message_id = src->last_message_id;
  counters->segments_count = src->segments_count;
  counters->segments_lost = src->segments_lost;
  counters->segments_reordered = src->segments_reordered;
  return counters;
}

void *t_monitoring(void *in)
{
  struct monitoring_thread_input *input = (struct monitoring_thread_input *)in;
  queue_t *output_queue = input->output_queue;
  struct seg_counters *counters = input->current;
  while (1)
  {
    sleep(input->delay);
    struct seg_counters *clone = clone_counters(counters);
    unyte_queue_destructive_write(output_queue, clone);
    reinit_counters(counters);
  }
  return NULL;
}
