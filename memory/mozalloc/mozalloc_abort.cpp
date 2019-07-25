







































#include <stdio.h>
#include <stdlib.h>             

#if defined(XP_WIN)
#  include <intrin.h>           
#elif defined(XP_UNIX)
#  include <unistd.h>           
#endif

#if defined(XP_WIN) || (defined(XP_OS2) && defined(__declspec))
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif

#include "mozilla/mozalloc_abort.h"

static int gDummyCounter;

static void
TouchBadMemory()
{
    
    gDummyCounter += *((int *) 0);   
                                     
}

void
mozalloc_abort(const char* const msg)
{
    fputs(msg, stderr);
    fputs("\n", stderr);

#if defined(XP_UNIX) && !defined(XP_MACOSX)
    abort();
#elif defined(XP_WIN)
    __debugbreak();
#endif
    
    

    
    TouchBadMemory();

    
    _exit(127);
}
