



#include <string.h>
#include "mozilla/Types.h"

#ifdef ANDROID
#define wrap(a) __wrap_ ## a
#endif

#if defined(XP_WIN) || defined(XP_MACOSX)
#define wrap(a) je_ ## a
#endif

#ifdef wrap
void *wrap(malloc)(size_t);

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
