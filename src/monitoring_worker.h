#ifndef H_MONITORING_WORKER
#define H_MONITORING_WORKER

#include <stdint.h>
#include "queue.h"

typedef struct seg_counters
{
  uint32_t last_generator_id;
  uint32_t last_message_id;
  uint32_t segments_count;
  uint32_t segments_lost;
  uint32_t segments_reordered;
} unyte_seg_counters_t;

struct monitoring_thread_input 
{
  struct seg_counters *current; // current counters
  queue_t *output_queue;        // output queue of stats
  uint delay;                   // in seconds
};

void update_lost_segment(struct seg_counters *counters, uint32_t last_gid, uint32_t last_mid);
void update_ok_segment(struct seg_counters *counters, uint32_t last_gid, uint32_t last_mid);
void update_reordered_segment(struct seg_counters *counters, uint32_t last_gid, uint32_t last_mid);
void reinit_counters(struct seg_counters *counters);
void *t_monitoring(void *in);

#endif
