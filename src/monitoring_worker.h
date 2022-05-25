#ifndef H_MONITORING_WORKER
#define H_MONITORING_WORKER

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "unyte_udp_queue.h"

typedef enum
{
  PARSER_WORKER,
  LISTENER_WORKER
} thread_type_t;

typedef struct active_gid
{
  uint32_t observation_domain_id; // generator id / observation id
  int active;            // if > ODID_TIME_TO_LIVE observation_domain_id considered not receiving anymore
} active_odid_t;

// linear probing
typedef struct odid_counter
{
  uint32_t observation_domain_id;
  uint32_t segments_received;
  uint32_t segments_dropped;
  uint32_t segments_reordered;
  uint32_t last_message_id;
  struct odid_counter *next;
} unyte_odid_counter_t;

typedef struct seg_counters
{
  pthread_t thread_id;                  // thread id
  unyte_odid_counter_t *odid_counters;  // hashmap array of n unyte_odid_counter_t
  thread_type_t type;                   // type of thread: PARSER_WORKER | LISTENER_WORKER
  active_odid_t *active_odids;          // active observation domain ids array
  uint active_odids_length;             // active observation domain used
  uint active_odids_max_length;         // active_gids max array length. Used to resize if active_gids is full
} unyte_seg_counters_t;

/**
 * exposed struct to user
 */
typedef struct counter_summary
{
  pthread_t thread_id;
  uint32_t observation_domain_id;
  uint32_t last_message_id;
  uint32_t segments_received;
  uint32_t segments_dropped;
  uint32_t segments_reordered;
  thread_type_t type;
} unyte_udp_sum_counter_t;

struct monitoring_thread_input
{
  unyte_seg_counters_t *counters;  // current counters for every thread (parsing workers + listening thread)
  uint nb_counters;                // number of counters
  unyte_udp_queue_t *output_queue; // output queue of stats
  uint delay;                      // in seconds
  bool stop_monitoring_thread;     // bool to stop thread
};

unyte_seg_counters_t *unyte_udp_init_counters(uint nb_threads);

/**
 * Updates counters with a dropped segment
 */
void unyte_udp_update_dropped_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid);

/**
 * Updates counters with a received segment
 */
void unyte_udp_update_received_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid);
void *t_monitoring_unyte_udp(void *in);
void unyte_udp_print_counters(unyte_udp_sum_counter_t *counter, FILE *std);
void unyte_udp_free_seg_counters(unyte_seg_counters_t *counters, uint nb_counter);

// Getters
pthread_t unyte_udp_get_thread_id(unyte_udp_sum_counter_t *counter);
uint32_t unyte_udp_get_od_id(unyte_udp_sum_counter_t *counter);
uint32_t unyte_udp_get_last_msg_id(unyte_udp_sum_counter_t *counter);
uint32_t unyte_udp_get_received_seg(unyte_udp_sum_counter_t *counter);
uint32_t unyte_udp_get_dropped_seg(unyte_udp_sum_counter_t *counter);
uint32_t unyte_udp_get_reordered_seg(unyte_udp_sum_counter_t *counter);
thread_type_t unyte_udp_get_th_type(unyte_udp_sum_counter_t *counter);

#endif
