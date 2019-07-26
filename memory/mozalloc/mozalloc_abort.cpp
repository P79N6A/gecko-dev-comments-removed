






#if defined(XP_WIN) || defined(XP_OS2)
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif

#include "mozilla/mozalloc_abort.h"

#ifdef ANDROID
# include <android/log.h>
#endif
#include <stdio.h>

#include "mozilla/Assertions.h"

void
mozalloc_abort(const char* const msg)
{
#ifndef ANDROID
    fputs(msg, stderr);
    fputs("\n", stderr);
#else
    __android_log_print(ANDROID_LOG_ERROR, "Gecko", "mozalloc_abort: %s", msg);
#endif
    MOZ_CRASH();
}

#if defined(XP_UNIX)



void abort(void)
{
    mozalloc_abort("Redirecting call to abort() to mozalloc_abort\n");
}
#endif

