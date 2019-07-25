







































#include <stdio.h>
#include <stdlib.h>             

#if defined(_MSC_VER)           
#  include <intrin.h>           
#elif defined(XP_WIN)           
#  include <windows.h>          
#elif defined(XP_UNIX)
#  include <unistd.h>           
#  include <signal.h>
#endif

#if defined(XP_WIN) || defined(XP_OS2)
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif

#include "mozilla/mozalloc_abort.h"

static int gDummyCounter;



MOZ_NEVER_INLINE static void
TouchBadMemory()
{
    
    volatile int *p = 0;
    gDummyCounter += *p;   
                           
}

void
mozalloc_abort(const char* const msg)
{
    fputs(msg, stderr);
    fputs("\n", stderr);

#if defined(_MSC_VER)
    __debugbreak();
#elif defined(XP_WIN)
    DebugBreak();
#endif

    
    
    
    

    TouchBadMemory();

    
    
    
    
    
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    raise(SIGABRT);
#endif

    
    _exit(127);
}

#if defined(XP_UNIX)



void abort(void)
{
  mozalloc_abort("Redirecting call to abort() to mozalloc_abort\n");
}
#endif

