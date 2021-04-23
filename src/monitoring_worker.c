#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include "monitoring_worker.h"

uint32_t hash_key(uint32_t gid)
{
  return (gid) % GID_COUNTERS;
}

void init_gid_counter(unyte_gid_counter_t *gid_counters)
{
  unyte_gid_counter_t *cur = gid_counters;
  cur->segments_count = 0;
  cur->segments_lost = 0;
  cur->segments_reordered = 0;
  cur->last_message_id = 0;
  cur->generator_id = 0;
  cur->next = NULL;
}

void reinit_gid_counters(unyte_gid_counter_t *gid_counter)
{
  gid_counter->last_message_id = 0;
  gid_counter->segments_count = 0;
  gid_counter->segments_lost = 0;
  gid_counter->segments_reordered = 0;
}

int init_active_gid_index(unyte_seg_counters_t *counters)
{
  counters->active_gids = (active_gid_t *)malloc(sizeof(active_gid_t) * ACTIVE_GIDS);
  if (counters->active_gids == NULL)
    return -1;
  counters->active_gids_length = 0;
  counters->active_gids_max_length = ACTIVE_GIDS;
  return 0;
}

int resize_active_gid_index(unyte_seg_counters_t *counters)
{
  active_gid_t *new_active_gid = (active_gid_t *)realloc(counters->active_gids, sizeof(active_gid_t) * (counters->active_gids_max_length + ACTIVE_GIDS));
  if (new_active_gid == NULL)
    return -1;
  counters->active_gids = new_active_gid;
  counters->active_gids_max_length = counters->active_gids_max_length + ACTIVE_GIDS;
  return 0;
}

unyte_seg_counters_t *init_counters(uint nb_threads)
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
    cur->gid_counters = malloc(sizeof(unyte_gid_counter_t) * GID_COUNTERS);
    if (cur->gid_counters == NULL)
    {
      printf("Malloc failed\n");
      return NULL;
    }
    for (uint o = 0; o < GID_COUNTERS; o++)
    {
      init_gid_counter(cur->gid_counters + o);
    }
    if (init_active_gid_index(cur) < 0)
    {
      printf("Malloc failed\n");
      return NULL;
    }
    cur++;
  }
  return counters;
}

/**
 * Returns existant gid_counter if exists, creates a new one if not existant
 * Saves the gid on a indexed array (active_gids)
 */
unyte_gid_counter_t *get_gid_counter(unyte_seg_counters_t *counters, uint32_t gid)
{
  unyte_gid_counter_t *cur = counters->gid_counters + hash_key(gid);
  while (cur->next != NULL)
  {
    cur = cur->next;
    if (cur->generator_id == gid)
    {
      return cur;
    }
  }
  // printf("Creating new one: %d|%d|%d\n", gid, hash_key(gid), cur == NULL);
  cur->next = (unyte_gid_counter_t *)malloc(sizeof(unyte_gid_counter_t));
  cur->next->generator_id = gid;
  cur->next->segments_count = 0;
  cur->next->segments_lost = 0;
  cur->next->segments_reordered = 0;
  cur->next->last_message_id = 0;
  cur->next->next = NULL;
  if (counters->active_gids_length + 1 >= counters->active_gids_max_length)
  {
    if (resize_active_gid_index(counters) < 0)
    {
      printf("Malloc failed.\n");
    }
  }
  // add gid to active gids
  counters->active_gids[counters->active_gids_length].generator_id = gid;
  counters->active_gids[counters->active_gids_length].active = 0;
  counters->active_gids_length += 1;
  return cur->next;
}

/**
 * Removes a gid from the linked list unyte_seg_counters_t and removes the gid from indexed array
 */
void remove_gid_counter(unyte_seg_counters_t *counters, uint32_t gid)
{
  unyte_gid_counter_t *cur = counters->gid_counters + hash_key(gid);
  unyte_gid_counter_t *next_counter;
  while (cur != NULL)
  {
    next_counter = cur->next;
    if (next_counter != NULL)
    {
      if (next_counter->generator_id == gid)
      {
        unyte_gid_counter_t *to_remove = next_counter;
        unyte_gid_counter_t *precedent = cur;
        precedent->next = next_counter->next;
        free(to_remove);
      }
    }
    cur = cur->next;
  }
  for (uint i = 0; i < counters->active_gids_length; i++)
  {
    if (counters->active_gids[i].generator_id == gid)
    {
      // Reordering array to reduce size
      for (uint o = i + 1; o < counters->active_gids_length; o++)
      {
        counters->active_gids[i].generator_id = counters->active_gids[o].generator_id;
        counters->active_gids[i].active = counters->active_gids[o].active;
      }
      counters->active_gids_length -= 1;
      break;
    }
  }
}

void update_lost_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid)
{
  unyte_gid_counter_t *gid_counter = get_gid_counter(counters, last_gid);
  gid_counter->segments_lost++;
  gid_counter->last_message_id = last_mid;
}

void update_ok_segment(unyte_seg_counters_t *counters, uint32_t last_gid, uint32_t last_mid)
{
  unyte_gid_counter_t *gid_counter = get_gid_counter(counters, last_gid);
  gid_counter->segments_count++;
  if (last_mid < gid_counter->last_message_id)
    gid_counter->segments_reordered++;
  gid_counter->last_message_id = last_mid;
}

void print_counters(unyte_sum_counter_t *counter, FILE *std)
{
  fprintf(std, "\n*** Counters ***\n");
  fprintf(std, "Thread: %lu\n", counter->thread_id);
  fprintf(std, "Type: %s\n", counter->type == PARSER_WORKER ? "PARSER_WORKER" : "LISTENER_WORKER");
  fprintf(std, "Generator id %d\n", counter->generator_id);
  fprintf(std, "OK: %u\n", counter->segments_count);
  fprintf(std, "Lost: %u\n", counter->segments_lost);
  fprintf(std, "Reorder: %u\n", counter->segments_reordered);
  fprintf(std, "Last msg id: %u\n", counter->last_message_id);
  fprintf(std, "***** End ******\n");
}

bool gid_counter_has_values(unyte_gid_counter_t *gid_counter)
{
  return !(gid_counter->last_message_id == 0 &&
           gid_counter->segments_count == 0 &&
           gid_counter->segments_lost == 0 &&
           gid_counter->segments_reordered == 0);
}

unyte_sum_counter_t *get_summary(unyte_gid_counter_t *gid_counter, pthread_t th_id, thread_type_t type)
{
  unyte_sum_counter_t *summary = (unyte_sum_counter_t *)malloc(sizeof(unyte_sum_counter_t));
  if (summary == NULL)
  {
    printf("get_summary(): Malloc error\n");
    return NULL;
  }
  summary->generator_id = gid_counter->generator_id;
  summary->last_message_id = gid_counter->last_message_id;
  summary->segments_count = gid_counter->segments_count;
  summary->segments_lost = gid_counter->segments_lost;
  summary->segments_reordered = gid_counter->segments_reordered;
  summary->thread_id = th_id;
  summary->type = type;
  return summary;
}

void *t_monitoring(void *in)
{
  struct monitoring_thread_input *input = (struct monitoring_thread_input *)in;
  queue_t *output_queue = input->output_queue;
  while (1)
  {
    sleep(input->delay);
    unyte_seg_counters_t *seg_counter_cur;
    for (uint i = 0; i < input->nb_counters; i++)
    {
      seg_counter_cur = (input->counters + i);
      uint active_gids = seg_counter_cur->active_gids_length;
      for (uint y = 0; y < active_gids; y++)
      {
        unyte_gid_counter_t *cur_gid_counter = get_gid_counter(seg_counter_cur, seg_counter_cur->active_gids[y].generator_id);
        if (gid_counter_has_values(cur_gid_counter))
        {
          unyte_sum_counter_t *summary = get_summary(cur_gid_counter, seg_counter_cur->thread_id, seg_counter_cur->type);
          if (summary != NULL)
          {
            unyte_queue_destructive_write(output_queue, summary);
            reinit_gid_counters(cur_gid_counter);
          }
          seg_counter_cur->active_gids[y].active = 0;
        }
        else if (seg_counter_cur->active_gids[y].active == GID_TIME_TO_LIVE)
        {
          remove_gid_counter(seg_counter_cur, seg_counter_cur->active_gids[y].generator_id);
        }
        else
        {
          unyte_sum_counter_t *summary = get_summary(cur_gid_counter, seg_counter_cur->thread_id, seg_counter_cur->type);
          if (summary != NULL)
          {
            unyte_queue_destructive_write(output_queue, summary);
            reinit_gid_counters(cur_gid_counter);
          }
          seg_counter_cur->active_gids[y].active += 1;
        }
      }
    }
  }
  return NULL;
}

/**
 * Free all malloc structs for unyte_seg_counters_t
 */
void free_seg_counters(unyte_seg_counters_t *counter, uint nb_counter)
{
  for (uint i = 0; i < nb_counter; i++)
  {
    free((counter + i)->active_gids);
    unyte_gid_counter_t *cur_gid = (counter + i)->gid_counters;
    for (uint o = 0; o < GID_COUNTERS; o++)
    {
      unyte_gid_counter_t *head = (cur_gid + o);
      unyte_gid_counter_t *cur_counter = head->next;
      unyte_gid_counter_t *tmp;
      while (cur_counter != NULL)
      {
        tmp = cur_counter->next;
        free(cur_counter);
        cur_counter = tmp;
      }
      free(cur_counter);
    }
    free(cur_gid);
  }
  free(counter);
}

pthread_t get_thread_id(unyte_sum_counter_t *counter) { return counter->thread_id; }
uint32_t get_gen_id(unyte_sum_counter_t *counter) { return counter->generator_id; }
uint32_t get_last_msg_id(unyte_sum_counter_t *counter) { return counter->last_message_id; }
uint32_t get_ok_seg(unyte_sum_counter_t *counter) { return counter->segments_count; }
uint32_t get_lost_seg(unyte_sum_counter_t *counter) { return counter->segments_lost; }
uint32_t get_reordered_seg(unyte_sum_counter_t *counter) { return counter->segments_reordered; }
thread_type_t get_th_type(unyte_sum_counter_t *counter) { return counter->type; }
