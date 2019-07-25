







































#if defined(XP_WIN) || defined(XP_OS2)
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif

#include "mozilla/mozalloc_abort.h"
#include "mozilla/mozalloc_oom.h"

void
mozalloc_handle_oom()
{
    
    
    
    mozalloc_abort("out of memory");
}
