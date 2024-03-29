






#include "mozilla/mozalloc_abort.h"

#ifdef ANDROID
# include <android/log.h>
#endif
#ifdef MOZ_WIDGET_ANDROID
# include "APKOpen.h"
# include "dlfcn.h"
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
#ifdef MOZ_WIDGET_ANDROID
    abortThroughJava(msg);
#endif
    MOZ_CRASH();
}

#ifdef MOZ_WIDGET_ANDROID
template <size_t N>
void fillAbortMessage(char (&msg)[N], uintptr_t retAddress) {
    




    Dl_info info = {};
    dladdr(reinterpret_cast<void*>(retAddress), &info);

    const char* const module = info.dli_fname ? info.dli_fname : "";
    const char* const base_module = strrchr(module, '/');
    const void* const module_offset =
        reinterpret_cast<void*>(retAddress - uintptr_t(info.dli_fbase));
    const char* const sym = info.dli_sname ? info.dli_sname : "";

    snprintf(msg, sizeof(msg), "abort() called from %s:%p (%s)",
             base_module ? base_module + 1 : module, module_offset, sym);
}
#endif

#if defined(XP_UNIX) && !defined(MOZ_ASAN)












void abort(void)
{
#ifdef MOZ_WIDGET_ANDROID
    char msg[64] = {};
    fillAbortMessage(msg, uintptr_t(__builtin_return_address(0)));
#else
    const char* const msg = "Redirecting call to abort() to mozalloc_abort\n";
#endif

    mozalloc_abort(msg);
}
#endif

