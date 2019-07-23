







































#include <stdlib.h>             

#if defined(_WIN32)
#  include <signal.h>           
#elif defined(XP_UNIX)
#  include <unistd.h>           
#endif

#include "mozilla/mozalloc_oom.h"

static int gDummyCounter;

void
mozalloc_handle_oom()
{
    
    
    

    
    
    
    

#if defined(_WIN32)
#  if !defined(WINCE)
    
    raise(SIGABRT);
#  endif
    
    _exit(3);
#elif defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
    abort();
#else
#  warning not attempting to abort() on this platform
#endif

    
    
    gDummyCounter += *((int*) 0); 
    
    
    
    _exit(127);
}
