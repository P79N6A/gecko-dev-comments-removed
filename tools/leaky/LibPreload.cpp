




































#include "TestPreload.h"
#include "config.h"
#include <malloc.h>





void* malloc(size_t aSize)
{
  if (aSize == LD_PRELOAD_TEST_MALLOC_SIZE) {
    return (void*) LD_PRELOAD_TEST_VALUE;
  }
  else {
    return REAL_MALLOC(aSize);
  }
}
