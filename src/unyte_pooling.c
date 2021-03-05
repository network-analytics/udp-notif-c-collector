#include <stdio.h>
#include "unyte_pooling.h"

struct unyte_pool *unyte_pool_init(uint buff_size)
{
  struct unyte_pool *pool = (struct unyte_pool *)malloc(sizeof(struct unyte_pool));
  if (pool == NULL) 
  {
    exit(1);
  }
  pool->size = 0;
  pool->head = NULL;
  pool->tail = NULL;
  // buff_size is const 
  *(uint *)&pool->buff_size = buff_size;
  pool->empties = NULL;
  return pool;
}

void *unyte_malloc(struct unyte_pool *pool)
{
  void *buff;
  if (pool->size == 0)
  {
    buff = malloc(pool->buff_size);
  } else {
    struct pooling_unit *unit;
    unit = pool->head;
    // update pool
    pool->head = unit->next;
    pool->size -= 1;
    // reset next 
    buff = unit->buffer;
    unit->next = NULL;
    unit->buffer = NULL;
    // save unit to reuse later
    if (pool->empties == NULL) {
      pool->empties = unit;
    } else {
      // appending at the head
      unit->next = pool->empties;
      pool->empties = unit;
    }
  }

  return buff;
}

struct pooling_unit *get_empty_unit(struct unyte_pool *pool) {
  if (pool->empties == NULL) {
    struct pooling_unit *p_unit = (struct pooling_unit *)malloc(sizeof(struct pooling_unit));
    p_unit->next = NULL;
    p_unit->buffer = NULL;
    return p_unit;
  } else {
    struct pooling_unit *unit = pool->empties;
    pool->empties = unit->next;
    unit->next = NULL;
    return unit;
  }
}

int unyte_free(struct unyte_pool *pool, void *buffer)
{
  if (pool == NULL) 
  {
    free(buffer);
    return -1;
  }
  if (pool->size >= MAX_POOL) 
  {
    free(buffer);
  } else if (pool->size == 0) {
    struct pooling_unit *empty_unit = get_empty_unit(pool);
    empty_unit->buffer = buffer;

    pool->head = empty_unit;
    pool->tail = empty_unit;
    pool->size += 1;
  } else {
    struct pooling_unit *empty_unit = get_empty_unit(pool);
    empty_unit->buffer = buffer;
    pool->tail->next = empty_unit;
    pool->tail = pool->tail->next;
    pool->size += 1;
  }
  return 0;
}

int unyte_free_pool(struct unyte_pool *pool) {
  if (pool->size > 0) {
    struct pooling_unit *cur = pool->head;
    struct pooling_unit *cleaning;
    while (cur != NULL) {
      // printf("Clear\n");
      cleaning = cur;
      cur = cur ->next;
      free(cleaning->buffer);
      free(cleaning);
    }
    cur = pool->empties;
    while(cur != NULL) {
      cleaning = cur;
      cur = cur->next;
      free(cleaning);
    }
  }
  free(pool);
  return 0;
}
