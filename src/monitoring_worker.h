#ifndef H_MONITORING_WORKER
#define H_MONITORING_WORKER

#include <stdint.h>
#include <stdio.h>
#include "queue.h"

#define GID_COUNTERS 10
#define ACTIVE_GIDS 500     // how many active gids do we are waiting for
#define GID_TIME_TO_LIVE 4  // times monitoring thread consider gid active without stats

typedef enum
{
  PARSER_WORKER,
  LISTENER_WORKER
} thread_type_t;

typedef struct active_gid
{
  uint32_t generator_id;
  int active;            // if > GID_TIME_TO_LIVE gen_id considered not receiving anymore
} active_gid_t;

typedef struct gid_counter
{
  uint32_t generator_id;
  uint32_t segments_count;
  uint32_t segments_lost;
  uint32_t segments_reordered;
  uint32_t last_message_id;
  struct gid_counter *next;
} unyte_gid_counter_t;

typedef struct seg_counters
{
  pthread_t thread_id;
  unyte_gid_counter_t *gid_counters;  // array of n unyte_gid_counter_t
  thread_type_t type;
  active_gid_t *active_gids;          // active generator ids
  uint active_gids_length;
  uint active_gids_max_length;        // active_gids max array length
} unyte_seg_counters_t;

/**
 * exposed struct to user
 */
typedef struct counter_summary
{
  pthread_t thread_id;
  uint32_t generator_id;
  uint32_t last_message_id;
  uint32_t segments_count;
  uint32_t segments_lost;
  uint32_t segments_reordered;
  thread_type_t type;
} unyte_sum_counter_t;

struct monitoring_thread_input
{
  unyte_seg_counters_t *counters; // current counters for every thread (parsing workers + listening thread)
  uint nb_counters;               // number of counters
  queue_t *output_queue;          // output queue of stats
  uint delay;                     // in seconds
};

unyte_seg_counters_t *init_counters(uint nb_threads);
void update_lost_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid);
void update_ok_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid);
void reinit_counters(unyte_seg_counters_t *counters);
void *t_monitoring(void *in);
void print_counters(unyte_sum_counter_t *counter, FILE *std);
void free_seg_counters(unyte_seg_counters_t *counters, uint nb_counter);

// Getters
pthread_t get_thread_id(unyte_sum_counter_t *counter);
uint32_t get_gen_id(unyte_sum_counter_t *counter);
uint32_t get_last_msg_id(unyte_sum_counter_t *counter);
uint32_t get_ok_seg(unyte_sum_counter_t *counter);
uint32_t get_lost_seg(unyte_sum_counter_t *counter);
uint32_t get_reordered_seg(unyte_sum_counter_t *counter);
thread_type_t get_th_type(unyte_sum_counter_t *counter);

#endif
