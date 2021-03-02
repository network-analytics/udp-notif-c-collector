#include <stdio.h>
#include <stdlib.h>

// void* tc_malloc(size_t size);
// void  tc_free(void* ptr);
// void *malloc(size_t size_t);

int main()
{
  char *test = (char *)malloc(12);
  // printf("test:%d\n", test == NULL);
  // printf("str:%s\n", test);
  test[0] = 'A';
  test[1] = 'B';
  test[2] = 'C';
  test[3] = 'D';
  test[4] = 'E';
  test[5] = 'F';
  test[6] = 'G';
  printf("str:%s\n", test);
  free(test);
  return 0;
}
