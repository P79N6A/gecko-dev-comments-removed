










































#include "prmem.h"
#include "prerror.h"
#include "string.h"

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
PORT_ZFree(void *ptr, size_t len)
{
  if (ptr) {
    memset(ptr, 0, len);
    PR_Free(ptr);
  }
}

void
PORT_SetError(int value)
{
  PR_SetError(value, 0);
  return;
}
