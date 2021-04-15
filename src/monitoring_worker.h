#ifndef H_MONITORING_WORKER
#define H_MONITORING_WORKER

#include <stdint.h>
#include <stdio.h>
#include "queue.h"

typedef struct seg_counters
{
  pthread_t thread_id;
  uint32_t last_generator_id;
  uint32_t last_message_id;
  uint32_t segments_count;
  uint32_t segments_lost;
  uint32_t segments_reordered;
} unyte_seg_counters_t;

struct monitoring_thread_input 
{
  unyte_seg_counters_t *counters; // current counters for every thread (parsing workers + listening thread)
  uint nb_counters;               // number of counters
  queue_t *output_queue;          // output queue of stats
  uint delay;                     // in seconds
};

unyte_seg_counters_t * init_counters(uint nb_threads);
void update_lost_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid);
void update_ok_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid);
void update_reordered_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid);
void reinit_counters(unyte_seg_counters_t *counters);
void *t_monitoring(void *in);
void print_counters(unyte_seg_counters_t *counter, FILE *std);

//TODO: getters ?

#endif
