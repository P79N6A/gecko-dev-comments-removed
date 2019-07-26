














































#include "ut_sim.h"


int
ut_compar(const void *a, const void *b) {
  return rand() > (RAND_MAX/2) ? -1 : 1;
}

void
ut_init(ut_connection *utc) {
  int i;
  utc->index = 0;

  for (i=0; i < UT_BUF; i++)
    utc->buffer[i] = i;
  
  qsort(utc->buffer, UT_BUF, sizeof(uint32_t), ut_compar);

  utc->index = UT_BUF - 1;
}

uint32_t
ut_next_index(ut_connection *utc) {
  uint32_t tmp;

  tmp = utc->buffer[0];
  utc->index++;
  utc->buffer[0] = utc->index;

  qsort(utc->buffer, UT_BUF, sizeof(uint32_t), ut_compar);
  
  return tmp;
}



#ifdef UT_TEST

#include <stdio.h>

int
main() {
  uint32_t i, irecvd, idiff;
  ut_connection utc;

  ut_init(&utc);

  for (i=0; i < 1000; i++) {
    irecvd = ut_next_index(&utc);
    idiff = i - irecvd;
    printf("%lu\t%lu\t%d\n", i, irecvd, idiff);
  }
  
  return 0;
}


#endif
