







































#include "mozilla/Attributes.h"
#include "mozilla/Types.h"

#include <cstdio>
#include <cstdlib>
#ifndef WIN32
#include <signal.h>
#endif



extern "C" {

MOZ_EXPORT_API(void)
MOZ_Crash()
{
  






#if defined(WIN32)
  



  *((volatile int *) NULL) = 123;
  exit(3);
#elif defined(__APPLE__)
  



  *((volatile int *) NULL) = 123;  
  raise(SIGABRT);  
#else
  raise(SIGABRT);  
#endif
}

MOZ_EXPORT_API(void)
MOZ_Assert(const char* s, const char* file, int ln)
{
  fprintf(stderr, "Assertion failure: %s, at %s:%d\n", s, file, ln);
  fflush(stderr);
  MOZ_Crash();
}

}
