



#ifdef MOZ_JEMALLOC3

#define MOZ_JEMALLOC_IMPL

#include "mozmemory_wrap.h"
#include "mozilla/Types.h"


#ifdef MOZ_WIDGET_GONK



#define MOZ_MALLOC_PLATFORM_OPTIONS ",lg_dirty_mult:8"
#else
#define MOZ_MALLOC_PLATFORM_OPTIONS ",lg_dirty_mult:6"
#endif

#ifdef DEBUG
#define MOZ_MALLOC_BUILD_OPTIONS ",junk:true"
#else
#define MOZ_MALLOC_BUILD_OPTIONS ",junk:free"
#endif

#define MOZ_MALLOC_OPTIONS "narenas:1,lg_chunk:20,tcache:false"
MFBT_DATA const char * je_(malloc_conf) =
  MOZ_MALLOC_OPTIONS MOZ_MALLOC_PLATFORM_OPTIONS MOZ_MALLOC_BUILD_OPTIONS;

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
