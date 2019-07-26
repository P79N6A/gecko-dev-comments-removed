



#include <string.h>
#include "mozilla/Types.h"

#ifdef MOZ_WIDGET_ANDROID
#define wrap(a) __wrap_ ## a
#elif defined(XP_WIN) || defined(XP_MACOSX)
#define wrap(a) je_ ## a
#elif defined(MOZ_WIDGET_GONK)
#define wrap(a) a
#endif

#ifdef wrap
void *wrap(malloc)(size_t);
void wrap(free)(void *);
#endif

#ifdef ANDROID

MOZ_EXPORT_API(void *)
wrap(_Znwj)(unsigned int size)
{
  return wrap(malloc)(size);
}

MOZ_EXPORT_API(void *)
wrap(_Znaj)(unsigned int size)
{
  return wrap(malloc)(size);
}

MOZ_EXPORT_API(void)
wrap(_ZdlPv)(void *ptr)
{
  wrap(free)(ptr);
}

MOZ_EXPORT_API(void)
wrap(_ZdaPv)(void *ptr)
{
  wrap(free)(ptr);
}

MOZ_EXPORT_API(void *)
wrap(_ZnwjRKSt9nothrow_t)(unsigned int size)
{
  return wrap(malloc)(size);
}

MOZ_EXPORT_API(void *)
wrap(_ZnajRKSt9nothrow_t)(unsigned int size)
{
  return wrap(malloc)(size);
}

MOZ_EXPORT_API(void)
wrap(_ZdlPvRKSt9nothrow_t)(void *ptr)
{
  wrap(free)(ptr);
}

MOZ_EXPORT_API(void)
wrap(_ZdaPvRKSt9nothrow_t)(void *ptr)
{
  wrap(free)(ptr);
}
#endif

#ifdef wrap
MOZ_EXPORT_API(char *)
wrap(strndup)(const char *src, size_t len)
{
  char* dst = (char*) wrap(malloc)(len + 1);
  if (dst)
    strncpy(dst, src, len + 1);
  return dst; 
}

MOZ_EXPORT_API(char *)
wrap(strdup)(const char *src)
{
  size_t len = strlen(src);
  return wrap(strndup)(src, len);
}

#ifdef XP_WIN





void
wrap(dumb_free_thunk)(void *ptr)
{
  return; 
}

#include <wchar.h>





wchar_t *
wrap(wcsdup)(const wchar_t *src)
{
  size_t len = wcslen(src);
  wchar_t *dst = (wchar_t*) wrap(malloc)((len + 1) * sizeof(wchar_t));
  if (dst)
    wcsncpy(dst, src, len + 1);
  return dst;
}
#endif 

#endif

#ifdef MOZ_JEMALLOC

const char *je_malloc_conf = "narenas:1,lg_chunk:20";

#ifdef ANDROID
#include <android/log.h>

static void
_je_malloc_message(void *cbopaque, const char *s)
{
  __android_log_print(ANDROID_LOG_INFO, "GeckoJemalloc", "%s", s);
}

void (*je_malloc_message)(void *, const char *s) = _je_malloc_message;
#endif
#endif

#include <mozilla/Assertions.h>

void moz_abort() {
  MOZ_CRASH();
}
