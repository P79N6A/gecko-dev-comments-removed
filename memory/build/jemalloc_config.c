



#ifdef MOZ_JEMALLOC3

#define MOZ_JEMALLOC_IMPL

#include "mozmemory_wrap.h"
#include "mozilla/Types.h"


MFBT_DATA const char * je_(malloc_conf) = "narenas:1,lg_chunk:20,tcache:false";

#ifdef ANDROID
#include <android/log.h>

static void
_je_malloc_message(void *cbopaque, const char *s)
{
  __android_log_print(ANDROID_LOG_INFO, "GeckoJemalloc", "%s", s);
}

void (*je_(malloc_message))(void *, const char *s) = _je_malloc_message;
#endif

#endif 


#include <mozilla/Assertions.h>

void moz_abort() {
  MOZ_CRASH();
}
