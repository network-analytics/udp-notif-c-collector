#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include "monitoring_worker.h"
#include "unyte_udp_defaults.h"

// TODO: new hashkey function + prepend unyte
uint32_t hash_key(uint32_t odid)
{
  return (odid) % ODID_COUNTERS;
}

/**
 * Initialize element unyte_odid_counter_t
 */
void init_odid_counter(unyte_odid_counter_t *odid_counters)
{
  unyte_odid_counter_t *cur = odid_counters;
  cur->segments_received = 0;
  cur->segments_dropped = 0;
  cur->segments_reordered = 0;
  cur->last_message_id = 0;
  cur->observation_domain_id = 0;
  cur->next = NULL;
}

/**
 * Reinitialize the counter for the element *odid_counter
 */
void reinit_odid_counters(unyte_odid_counter_t *odid_counter)
{
  odid_counter->last_message_id = 0;
  odid_counter->segments_received = 0;
  odid_counter->segments_dropped = 0;
  odid_counter->segments_reordered = 0;
}

/**
 * Initialize active obervation domain id index array.
 */
int init_active_odid_index(unyte_seg_counters_t *counters)
{
  counters->active_odids = (active_odid_t *)malloc(sizeof(active_odid_t) * ACTIVE_GIDS);
  if (counters->active_odids == NULL)
    return -1;
  counters->active_odids_length = 0;
  counters->active_odids_max_length = ACTIVE_GIDS;
  return 0;
}

/**
 * Resize active observation domain id index array if it is full
 */
int resize_active_odid_index(unyte_seg_counters_t *counters)
{
  active_odid_t *new_active_odid = (active_odid_t *)realloc(counters->active_odids, sizeof(active_odid_t) * (counters->active_odids_max_length + ACTIVE_GIDS));
  if (new_active_odid == NULL)
    return -1;
  counters->active_odids = new_active_odid;
  counters->active_odids_max_length = counters->active_odids_max_length + ACTIVE_GIDS;
  return 0;
}

/**
 * Initialize counters for <nb_threads> threads
 */
unyte_seg_counters_t *unyte_udp_init_counters(uint nb_threads)
{
  unyte_seg_counters_t *counters = (unyte_seg_counters_t *)malloc(sizeof(unyte_seg_counters_t) * (nb_threads));
  if (counters == NULL)
  {
    printf("Malloc failed\n");
    return NULL;
  }
  unyte_seg_counters_t *cur = counters;
  for (uint i = 0; i < nb_threads; i++)
  {
    cur->odid_counters = malloc(sizeof(unyte_odid_counter_t) * ODID_COUNTERS);
    if (cur->odid_counters == NULL)
    {
      printf("Malloc failed\n");
      return NULL;
    }
    for (uint o = 0; o < ODID_COUNTERS; o++)
    {
      init_odid_counter(cur->odid_counters + o);
    }
    if (init_active_odid_index(cur) < 0)
    {
      printf("Malloc failed\n");
      return NULL;
    }
    cur++;
  }
  return counters;
}

/**
 * Returns existant odid_counter if exists, creates a new one if not existant
 * Saves the odid on a indexed array (active_odids)
 */
unyte_odid_counter_t *get_odid_counter(unyte_seg_counters_t *counters, uint32_t odid)
{
  // get hashmap element from global counters hashmap
  unyte_odid_counter_t *cur = counters->odid_counters + hash_key(odid);
  // get element : linear probing
  while (cur->next != NULL)
  {
    cur = cur->next;
    if (cur->observation_domain_id == odid)
      return cur;
  }
  // Creates a new one on last element of linear probing linked list
  cur->next = (unyte_odid_counter_t *)malloc(sizeof(unyte_odid_counter_t));
  // Malloc failed
  if (cur->next == NULL)
    return NULL;
  cur->next->observation_domain_id = odid;
  cur->next->segments_received = 0;
  cur->next->segments_dropped = 0;
  cur->next->segments_reordered = 0;
  cur->next->last_message_id = 0;
  cur->next->next = NULL;
  // resize active observation domain id index array if it is full
  if (counters->active_odids_length + 1 >= counters->active_odids_max_length)
  {
    if (resize_active_odid_index(counters) < 0)
      printf("Malloc failed.\n");
  }
  // add odid to active odids
  counters->active_odids[counters->active_odids_length].observation_domain_id = odid;
  counters->active_odids[counters->active_odids_length].active = 0;
  counters->active_odids_length += 1;
  return cur->next;
}

/**
 * Removes a odid from the linked list unyte_seg_counters_t and removes the odid from indexed array
 */
void remove_odid_counter(unyte_seg_counters_t *counters, uint32_t odid)
{
  unyte_odid_counter_t *cur = counters->odid_counters + hash_key(odid);
  unyte_odid_counter_t *next_counter;
  while (cur != NULL)
  {
    next_counter = cur->next;
    if (next_counter != NULL)
    {
      if (next_counter->observation_domain_id == odid)
      {
        unyte_odid_counter_t *to_remove = next_counter;
        unyte_odid_counter_t *precedent = cur;
        precedent->next = next_counter->next;
        free(to_remove);
      }
    }
    cur = cur->next;
  }
  for (uint i = 0; i < counters->active_odids_length; i++)
  {
    if (counters->active_odids[i].observation_domain_id == odid)
    {
      // Reordering array to reduce size
      for (uint o = i + 1; o < counters->active_odids_length; o++)
      {
        counters->active_odids[i].observation_domain_id = counters->active_odids[o].observation_domain_id;
        counters->active_odids[i].active = counters->active_odids[o].active;
      }
      counters->active_odids_length -= 1;
      break;
    }
  }
}

void unyte_udp_update_dropped_segment(unyte_seg_counters_t *counters, uint32_t last_odid, uint32_t last_mid)
{
  unyte_odid_counter_t *odid_counter = get_odid_counter(counters, last_odid);
  if (odid_counter == NULL)
    printf("Malloc failed\n");
  else
  {
    odid_counter->segments_dropped++;
    odid_counter->last_message_id = last_mid;
  }
}

void unyte_udp_update_received_segment(unyte_seg_counters_t *counters, uint32_t last_odid, uint32_t last_mid)
{
  unyte_odid_counter_t *odid_counter = get_odid_counter(counters, last_odid);
  if (odid_counter == NULL)
    printf("Malloc failed\n");
  else
  {
    odid_counter->segments_received++;
    if (last_mid < odid_counter->last_message_id)
      odid_counter->segments_reordered++;
    else
      odid_counter->last_message_id = last_mid;
  }
}

void unyte_udp_print_counters(unyte_udp_sum_counter_t *counter, FILE *std)
{
  fprintf(std, "th_id:%10lu|gen_id:%5u|type: %15s|received:%7u|dropped:%7u|reordered:%5u|last_msg_id:%9u\n",
          counter->thread_id,
          counter->observation_domain_id,
          counter->type == PARSER_WORKER ? "PARSER_WORKER" : "LISTENER_WORKER",
          counter->segments_received,
          counter->segments_dropped,
          counter->segments_reordered,
          counter->last_message_id);
}

/**
 * Checks if *odid_counter has valid values to clone
 */
bool odid_counter_has_values(unyte_odid_counter_t *odid_counter)
{
  return (odid_counter != NULL) && !(odid_counter->last_message_id == 0 &&
                                    odid_counter->segments_received == 0 &&
                                    odid_counter->segments_dropped == 0 &&
                                    odid_counter->segments_reordered == 0);
}

/**
 * Clone *odid_counter to send to user client
 */
unyte_udp_sum_counter_t *get_summary(unyte_odid_counter_t *odid_counter, pthread_t th_id, thread_type_t type)
{
  if (odid_counter == NULL)
    return NULL;

  unyte_udp_sum_counter_t *summary = (unyte_udp_sum_counter_t *)malloc(sizeof(unyte_udp_sum_counter_t));
  if (summary == NULL)
  {
    printf("get_summary(): Malloc error\n");
    return NULL;
  }
  summary->observation_domain_id = odid_counter->observation_domain_id;
  summary->last_message_id = odid_counter->last_message_id;
  summary->segments_received = odid_counter->segments_received;
  summary->segments_dropped = odid_counter->segments_dropped;
  summary->segments_reordered = odid_counter->segments_reordered;
  summary->thread_id = th_id;
  summary->type = type;
  return summary;
}

void *t_monitoring_unyte_udp(void *in)
{
  struct monitoring_thread_input *input = (struct monitoring_thread_input *)in;
  unyte_udp_queue_t *output_queue = input->output_queue;
  while (!input->stop_monitoring_thread)
  {
    sleep(input->delay);
    unyte_seg_counters_t *seg_counter_cur;
    // every thread (parsing_worker + listening_worker)
    for (uint i = 0; i < input->nb_counters; i++)
    {
      seg_counter_cur = (input->counters + i);
      uint active_odids = seg_counter_cur->active_odids_length;
      // every observation domain id read on the worker thread
      // seg_counter_cur->active_odids is the indexed observation domain ids received
      for (uint y = 0; y < active_odids; y++)
      {
        unyte_odid_counter_t *cur_odid_counter = get_odid_counter(seg_counter_cur, seg_counter_cur->active_odids[y].observation_domain_id);
        // if non-zero values, clone stats and send to queue
        if (odid_counter_has_values(cur_odid_counter))
        {
          unyte_udp_sum_counter_t *summary = get_summary(cur_odid_counter, seg_counter_cur->thread_id, seg_counter_cur->type);
          if (summary != NULL)
          {
            unyte_udp_queue_destructive_write(output_queue, summary);
            reinit_odid_counters(cur_odid_counter);
          }
          seg_counter_cur->active_odids[y].active = 0;
        }
        // if observation domain id has zeros-values and read ODID_TIME_TO_LIVE, remove structs from thread stats
        else if (seg_counter_cur->active_odids[y].active == ODID_TIME_TO_LIVE)
        {
          remove_odid_counter(seg_counter_cur, seg_counter_cur->active_odids[y].observation_domain_id);
        }
        // counters are zero and read < ODID_TIME_TO_LIVE, send to queue and active++
        else
        {
          unyte_udp_sum_counter_t *summary = get_summary(cur_odid_counter, seg_counter_cur->thread_id, seg_counter_cur->type);
          if (summary != NULL)
          {
            unyte_udp_queue_destructive_write(output_queue, summary);
            reinit_odid_counters(cur_odid_counter);
          }
          seg_counter_cur->active_odids[y].active += 1;
        }
      }
    }
  }
  pthread_exit(NULL);
}

/**
 * Free all malloc structs for unyte_seg_counters_t
 */
void unyte_udp_free_seg_counters(unyte_seg_counters_t *counter, uint nb_counter)
{
  for (uint i = 0; i < nb_counter; i++)
  {
    free((counter + i)->active_odids);
    unyte_odid_counter_t *cur_odid = (counter + i)->odid_counters;
    for (uint o = 0; o < ODID_COUNTERS; o++)
    {
      unyte_odid_counter_t *head = (cur_odid + o);
      unyte_odid_counter_t *cur_counter = head->next;
      unyte_odid_counter_t *tmp;
      while (cur_counter != NULL)
      {
        tmp = cur_counter->next;
        free(cur_counter);
        cur_counter = tmp;
      }
      free(cur_counter);
    }
    free(cur_odid);
  }
  free(counter);
}

// Getters
pthread_t unyte_udp_get_thread_id(unyte_udp_sum_counter_t *counter) { return counter->thread_id; }
uint32_t unyte_udp_get_od_id(unyte_udp_sum_counter_t *counter) { return counter->observation_domain_id; }
uint32_t unyte_udp_get_last_msg_id(unyte_udp_sum_counter_t *counter) { return counter->last_message_id; }
uint32_t unyte_udp_get_received_seg(unyte_udp_sum_counter_t *counter) { return counter->segments_received; }
uint32_t unyte_udp_get_dropped_seg(unyte_udp_sum_counter_t *counter) { return counter->segments_dropped; }
uint32_t unyte_udp_get_reordered_seg(unyte_udp_sum_counter_t *counter) { return counter->segments_reordered; }
thread_type_t unyte_udp_get_th_type(unyte_udp_sum_counter_t *counter) { return counter->type; }
