









#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "bbs_rand.h"

#define NUM_TESTS   100

int main(void)
{
  unsigned int   seed, result, ix;

  seed = time(NULL);
  bbs_srand((unsigned char *)&seed, sizeof(seed));

  for(ix = 0; ix < NUM_TESTS; ix++) {
    result = bbs_rand();
    
    printf("Test %3u: %08X\n", ix + 1, result);
  }

  return 0;
}
