




#include <stdio.h>
#include <errno.h>
#if defined(XP_WIN)
#include <windows.h>
#endif 

#include "jsctypes-test-errno.h"



#define FAIL \
{ \
  fprintf(stderr, "Assertion failed at line %i\n", __LINE__); \
  (*(int*)nullptr)++; \
}


void set_errno(int status)
{
  errno = status;
}
int get_errno()
{
  return errno;
}

#if defined(XP_WIN)
void set_last_error(int status)
{
  SetLastError((int)status);
}
int get_last_error()
{
  return (int)GetLastError();
}
#endif 
