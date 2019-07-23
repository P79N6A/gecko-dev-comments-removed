










































#include "prmem.h"
#include "prerror.h"

void*
PORT_Alloc(size_t bytes)
{
  
  return (void *)PR_Malloc(bytes ? bytes : 1);
}

void
PORT_Free(void *ptr)
{
  if (ptr) {
    PR_Free(ptr);
  }
}

void
PORT_SetError(int value)
{
  PR_SetError(value, 0);
  return;
}
