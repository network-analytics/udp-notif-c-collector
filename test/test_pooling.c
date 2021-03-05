#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/unyte_pooling.h"

struct test_unyte
{
  int size;
  int test_size;
  long lsixe;
  long tab[1000];
};

void test1() {

  struct unyte_pool *pool = unyte_pool_init(sizeof(struct test_unyte));

  // 1
  struct pooling_unit *unit = unyte_malloc(pool);
  printf("next:%d\n", unit->next == NULL);
  struct test_unyte *buff = (struct test_unyte *)unit->buffer;
  buff->size = 10;
  buff->test_size=10;
  printf("Buff:%d|%d\n", buff->size, buff->test_size);
  
  // 2
  struct pooling_unit *unit2 = unyte_malloc(pool);
  struct pooling_unit *unit3 = unyte_malloc(pool);
  struct pooling_unit *unit4 = unyte_malloc(pool);
  struct pooling_unit *unit5 = unyte_malloc(pool);
  struct pooling_unit *unit6 = unyte_malloc(pool);
  struct pooling_unit *unit7 = unyte_malloc(pool);
  struct pooling_unit *unit8 = unyte_malloc(pool);
  struct pooling_unit *unit9 = unyte_malloc(pool);
  struct pooling_unit *unit10 = unyte_malloc(pool);
  struct pooling_unit *unit11 = unyte_malloc(pool);
  struct pooling_unit *unit12 = unyte_malloc(pool);
  
  unyte_free(pool, unit);
  unyte_free(pool, unit2);
  unyte_free(pool, unit3);
  unyte_free(pool, unit4);
  unyte_free(pool, unit5);
  unyte_free(pool, unit6);
  unyte_free(pool, unit7);
  unyte_free(pool, unit8);
  unyte_free(pool, unit9);
  unyte_free(pool, unit10);
  unyte_free(pool, unit11);
  unyte_free(pool, unit12);
  printf("pool-size1:%d\n", pool->size);

  struct pooling_unit *unit13 = (struct pooling_unit *) unyte_malloc(pool);
  struct pooling_unit *unit14 = unyte_malloc(pool);
  struct pooling_unit *unit15 = unyte_malloc(pool);
  struct pooling_unit *unit16 = unyte_malloc(pool);
  struct pooling_unit *unit17 = unyte_malloc(pool);
  printf("pool-size2:%d\n", pool->size);


  // n
  struct pooling_unit *unitn = unyte_malloc(pool);
  printf("pool-size3:%d\n", pool->size);
  struct test_unyte *buffn = (struct test_unyte *)unitn->buffer;
  buffn->size = 10;
  buffn->test_size=10;
  printf("Buffn:%d|%d\n", buffn->size, buffn->test_size);

  free(buffn);
  free(unitn);

  free(unit13->buffer);
  free(unit14->buffer);
  free(unit15->buffer);
  free(unit16->buffer);
  free(unit17->buffer);

  free(unit13);
  free(unit14);
  free(unit15);
  free(unit16);
  free(unit17);

  unyte_free_pool(pool);
}

/**
 * Tests feature pooling
 */
void test2() {
  struct unyte_pool *pool = unyte_pool_init(sizeof(struct test_unyte));

  // 1
  struct test_unyte *buff = unyte_malloc(pool);
  buff->size =10;
  buff->test_size = 12;

  printf("Buff:%d|%d\n", buff->size, buff->test_size);

  // 2
  struct test_unyte *unit2 = unyte_malloc(pool);
  struct test_unyte *unit3 = unyte_malloc(pool);
  struct test_unyte *unit4 = unyte_malloc(pool);
  struct test_unyte *unit5 = unyte_malloc(pool);
  struct test_unyte *unit6 = unyte_malloc(pool);
  struct test_unyte *unit7 = unyte_malloc(pool);
  struct test_unyte *unit8 = unyte_malloc(pool);
  struct test_unyte *unit9 = unyte_malloc(pool);
  struct test_unyte *unit10 = unyte_malloc(pool);
  struct test_unyte *unit11 = unyte_malloc(pool);
  struct test_unyte *unit12 = unyte_malloc(pool);
  
  unyte_free(pool, buff);
  unyte_free(pool, unit2);
  unyte_free(pool, unit3);
  unyte_free(pool, unit4);
  unyte_free(pool, unit5);
  unyte_free(pool, unit6);
  unyte_free(pool, unit7);
  unyte_free(pool, unit8);
  unyte_free(pool, unit9);
  unyte_free(pool, unit10);
  unyte_free(pool, unit11);
  unyte_free(pool, unit12);
  printf("pool-size1:%d\n", pool->size);

  struct test_unyte *unit13 = unyte_malloc(pool);
  struct test_unyte *unit14 = unyte_malloc(pool);
  struct test_unyte *unit15 = unyte_malloc(pool);
  struct test_unyte *unit16 = unyte_malloc(pool);
  struct test_unyte *unit17 = unyte_malloc(pool);
  printf("pool-size2:%d\n", pool->size);

  unyte_free(pool, unit13);
  int count = 0;
  struct pooling_unit *cur = pool->empties;
  while(cur != NULL) {
    count++;
    cur = cur->next;
  }
  printf("Count:%d\n", count);
  // free(unit13);
  free(unit14);
  free(unit15);
  free(unit16);
  free(unit17);

  unyte_free_pool(pool);
}

void time_diff(struct timespec *diff, struct timespec *stop, struct timespec *start)
{
  if (stop->tv_nsec < start->tv_nsec)
  {
    /* here we assume (stop->tv_sec - start->tv_sec) is not zero */
    diff->tv_sec = stop->tv_sec - start->tv_sec - 1;
    diff->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
  }
  else
  {
    diff->tv_sec = stop->tv_sec - start->tv_sec;
    diff->tv_nsec = stop->tv_nsec - start->tv_nsec;
  }
  printf("%ld,%06ld\n", diff->tv_sec * 1000 + diff->tv_nsec / 1000000, diff->tv_nsec % 1000000);
}

struct test_unyte *unyte_malloc_test(struct unyte_pool *pool){
  struct test_unyte *buff = unyte_malloc(pool);
  buff->size = 10;
  buff->test_size = 12;
  return buff;
}
struct test_unyte *malloc_test()
{
  struct test_unyte *buff = malloc(sizeof(struct test_unyte));
  buff->size = 10;
  buff->test_size = 12;
  return buff;
}
/**
 * Tests performance diff between native malloc vs unyte_pooling
 */
void test3() 
{
  struct timespec start;
  struct timespec stop;
  struct timespec diff;
  int tests = 2000000;
  int count = 0;

  clock_gettime(CLOCK_MONOTONIC, &start);
  // pooling
  struct unyte_pool *pool = unyte_pool_init(sizeof(struct test_unyte));

  while(++count < tests) {
    struct test_unyte *buff = unyte_malloc_test(pool);
    struct test_unyte *buff2 = unyte_malloc_test(pool);
    struct test_unyte *buff3 = unyte_malloc_test(pool);
    struct test_unyte *buff4 = unyte_malloc_test(pool);
    struct test_unyte *buff5 = unyte_malloc_test(pool);
    struct test_unyte *buff6 = unyte_malloc_test(pool);
    struct test_unyte *buff7 = unyte_malloc_test(pool);
    struct test_unyte *buff8 = unyte_malloc_test(pool);
    struct test_unyte *buff9 = unyte_malloc_test(pool);
    struct test_unyte *buff10 = unyte_malloc_test(pool);
    struct test_unyte *buff11 = unyte_malloc_test(pool);
    struct test_unyte *buff12 = unyte_malloc_test(pool);
    struct test_unyte *buff13 = unyte_malloc_test(pool);
    struct test_unyte *buff14 = unyte_malloc_test(pool);
    struct test_unyte *buff15 = unyte_malloc_test(pool);
    struct test_unyte *buff16 = unyte_malloc_test(pool);
    struct test_unyte *buff17 = unyte_malloc_test(pool);
    struct test_unyte *buff18 = unyte_malloc_test(pool);
    struct test_unyte *buff19 = unyte_malloc_test(pool);
    struct test_unyte *buff20 = unyte_malloc_test(pool);

    unyte_free(pool, buff);
    unyte_free(pool, buff2);
    unyte_free(pool, buff3);
    unyte_free(pool, buff4);
    unyte_free(pool, buff5);
    unyte_free(pool, buff6);
    unyte_free(pool, buff7);
    unyte_free(pool, buff8);
    unyte_free(pool, buff9);
    unyte_free(pool, buff10);
    unyte_free(pool, buff11);
    unyte_free(pool, buff12);
    unyte_free(pool, buff13);
    unyte_free(pool, buff14);
    unyte_free(pool, buff15);
    unyte_free(pool, buff16);
    unyte_free(pool, buff17);
    unyte_free(pool, buff18);
    unyte_free(pool, buff19);
    unyte_free(pool, buff20);
  }
  clock_gettime(CLOCK_MONOTONIC, &stop);
  time_diff(&diff, &stop, &start);

  // malloc
  count = 0;
  clock_gettime(CLOCK_MONOTONIC, &start);
  while(++count < tests) {
    struct test_unyte *buff = malloc_test();
    struct test_unyte *buff2 = malloc_test();
    struct test_unyte *buff3 = malloc_test();
    struct test_unyte *buff4 = malloc_test();
    struct test_unyte *buff5 = malloc_test();
    struct test_unyte *buff6 = malloc_test();
    struct test_unyte *buff7 = malloc_test();
    struct test_unyte *buff8 = malloc_test();
    struct test_unyte *buff9 = malloc_test();
    struct test_unyte *buff10 = malloc_test();
    struct test_unyte *buff11 = malloc_test();
    struct test_unyte *buff12 = malloc_test();
    struct test_unyte *buff13 = malloc_test();
    struct test_unyte *buff14 = malloc_test();
    struct test_unyte *buff15 = malloc_test();
    struct test_unyte *buff16 = malloc_test();
    struct test_unyte *buff17 = malloc_test();
    struct test_unyte *buff18 = malloc_test();
    struct test_unyte *buff19 = malloc_test();
    struct test_unyte *buff20 = malloc_test();

    free(buff);
    free(buff2);
    free(buff3);
    free(buff4);
    free(buff5);
    free(buff6);
    free(buff7);
    free(buff8);
    free(buff9);
    free(buff10);
    free(buff11);
    free(buff12);
    free(buff13);
    free(buff14);
    free(buff15);
    free(buff16);
    free(buff17);
    free(buff18);
    free(buff19);
    free(buff20);
  }
  clock_gettime(CLOCK_MONOTONIC, &stop);
  time_diff(&diff, &stop, &start);
  unyte_free_pool(pool);
}

int main()
{
  //TODO: 1500 de payload de message without segmentation + tcmalloc
  //TODO: test tcmalloc on c-collector + jemalloc
  //TODO: test with 0 bytes of payload
  //TODO: parameters for c-collector instead of const
  //TODO: counters/thread/src:port (delivered/dropped/bytes_sent)
  // test1();
  test2();
  // test3();

  return 0;
}
