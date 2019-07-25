






#include "mozilla/Assertions.h"

#include <stdio.h>

#include "mozilla/mozalloc_abort.h"

void
mozalloc_abort(const char* const msg)
{
    fputs(msg, stderr);
    fputs("\n", stderr);
    MOZ_CRASH();
}

#if defined(XP_UNIX)



void abort(void)
{
    mozalloc_abort("Redirecting call to abort() to mozalloc_abort\n");
}
#endif

