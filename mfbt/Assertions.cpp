







































#include "mozilla/Attributes.h"
#include "mozilla/Types.h"

#include <cstdio>
#include <cstdlib>
#ifndef WIN32
#include <signal.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif



extern "C" {

MOZ_EXPORT_API(void)
MOZ_Crash()
{
  






#if defined(WIN32)
  



  *((volatile int *) NULL) = 123;
  exit(3);
#elif defined(ANDROID)
  



  *((volatile int *) NULL) = 123;
  abort();
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
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_FATAL, "MOZ_Assert",
                      "Assertion failure: %s, at %s:%d\n", s, file, ln);
#else
  fprintf(stderr, "Assertion failure: %s, at %s:%d\n", s, file, ln);
  fflush(stderr);
#endif
  MOZ_Crash();
}

}
