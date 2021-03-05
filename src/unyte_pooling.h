
#ifndef H_UNYTE_POOLING
#define H_UNYTE_POOLING

#include <stdlib.h>

#define MAX_POOL 2000

struct pooling_unit
{
  void *buffer;
  struct pooling_unit *next;
};

struct unyte_pool
{
  const uint buff_size;         // buffer size
  struct pooling_unit *head;    // head of pooling unit
  struct pooling_unit *tail;    // tail of pooling unit
  uint size;                    // actual pooling_unit presents on linked-list
  struct pooling_unit *empties; // empties pooling_units to reuse
};

struct unyte_pool *unyte_pool_init(uint buff_size);
void *unyte_malloc(struct unyte_pool *pool);

int unyte_free(struct unyte_pool *pool, void *unit);
int unyte_free_pool(struct unyte_pool *pool);

#endif
