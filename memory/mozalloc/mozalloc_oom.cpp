







































#if defined(XP_WIN) || defined(XP_OS2)
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif

#include "mozilla/mozalloc_abort.h"
#include "mozilla/mozalloc_oom.h"

static mozalloc_oom_abort_handler gAbortHandler;

void
mozalloc_handle_oom(size_t size)
{
    
    
    

    if (gAbortHandler)
        gAbortHandler(size);

    mozalloc_abort("out of memory");
}

void
mozalloc_set_oom_abort_handler(mozalloc_oom_abort_handler handler)
{
    gAbortHandler = handler;
}
