






#if defined(XP_WIN) || defined(XP_OS2)
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif

#include "mozilla/mozalloc_abort.h"
#include "mozilla/mozalloc_oom.h"
#include "mozilla/Assertions.h"

static mozalloc_oom_abort_handler gAbortHandler;

#define OOM_MSG_LEADER "out of memory: 0x"
#define OOM_MSG_DIGITS "0000000000000000" // large enough for 2^64
#define OOM_MSG_TRAILER " bytes requested"
#define OOM_MSG_FIRST_DIGIT_OFFSET sizeof(OOM_MSG_LEADER) - 1
#define OOM_MSG_LAST_DIGIT_OFFSET sizeof(OOM_MSG_LEADER) + \
                                  sizeof(OOM_MSG_DIGITS) - 3

static const char *hex = "0123456789ABCDEF";

void
mozalloc_handle_oom(size_t size)
{
    char oomMsg[] = OOM_MSG_LEADER OOM_MSG_DIGITS OOM_MSG_TRAILER;
    size_t i;

    
    
    

    if (gAbortHandler)
        gAbortHandler(size);

    MOZ_STATIC_ASSERT(OOM_MSG_FIRST_DIGIT_OFFSET > 0,
                      "Loop below will never terminate (i can't go below 0)");

    
    for (i = OOM_MSG_LAST_DIGIT_OFFSET;
         size && i >= OOM_MSG_FIRST_DIGIT_OFFSET; i--) {
      oomMsg[i] = hex[size % 16];
      size /= 16;
    }

    mozalloc_abort(oomMsg);
}

void
mozalloc_set_oom_abort_handler(mozalloc_oom_abort_handler handler)
{
    gAbortHandler = handler;
}
