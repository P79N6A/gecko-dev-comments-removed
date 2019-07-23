



































#include "TestPreload.h"
#include <malloc.h>
#include <stdio.h>







int main(int argc, char** argv)
{
  char* p = (char*) malloc(LD_PRELOAD_TEST_MALLOC_SIZE);
  if (p == (char*)LD_PRELOAD_TEST_VALUE) {
    printf("LD_PRELOAD worked - we are using our malloc\n");
  }
  else {
    printf("LD_PRELOAD failed\n");
  }
  return 0;
}
