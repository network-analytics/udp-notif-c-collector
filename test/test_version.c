#include <stdio.h>
#include <stdlib.h>
#include "../src/unyte_udp_collector.h"

int main()
{
  char *test = unyte_udp_notif_version();
  printf("Version: %s\n", test);
  free(test);
  return 0;
}
