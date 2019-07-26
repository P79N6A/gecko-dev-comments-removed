



#include <string.h>
#include "mozmemory_wrap.h"
#include "mozilla/Types.h"



#define MALLOC_DECL(name, return_type, ...) \
  MOZ_MEMORY_API return_type name ## _impl(__VA_ARGS__);
#include "malloc_decls.h"

#ifdef MOZ_WRAP_NEW_DELETE

MOZ_MEMORY_API void *
mozmem_malloc_impl(_Znwj)(unsigned int size)
{
  return malloc_impl(size);
}

MOZ_MEMORY_API void *
mozmem_malloc_impl(_Znaj)(unsigned int size)
{
  return malloc_impl(size);
}

MOZ_MEMORY_API void
mozmem_malloc_impl(_ZdlPv)(void *ptr)
{
  free_impl(ptr);
}

MOZ_MEMORY_API void
mozmem_malloc_impl(_ZdaPv)(void *ptr)
{
  free_impl(ptr);
}

MOZ_MEMORY_API void *
mozmem_malloc_impl(_ZnwjRKSt9nothrow_t)(unsigned int size)
{
  return malloc_impl(size);
}

MOZ_MEMORY_API void *
mozmem_malloc_impl(_ZnajRKSt9nothrow_t)(unsigned int size)
{
  return malloc_impl(size);
}

MOZ_MEMORY_API void
mozmem_malloc_impl(_ZdlPvRKSt9nothrow_t)(void *ptr)
{
  free_impl(ptr);
}

MOZ_MEMORY_API void
mozmem_malloc_impl(_ZdaPvRKSt9nothrow_t)(void *ptr)
{
  free_impl(ptr);
}
#endif



#undef strndup
#undef strdup

#ifndef XP_DARWIN
MOZ_MEMORY_API char *
strndup_impl(const char *src, size_t len)
{
  char* dst = (char*) malloc_impl(len + 1);
  if (dst) {
    strncpy(dst, src, len);
    dst[len] = '\0';
  }
  return dst;
}

MOZ_MEMORY_API char *
strdup_impl(const char *src)
{
  size_t len = strlen(src);
  return strndup_impl(src, len);
}
#endif 

#ifdef XP_WIN





void
dumb_free_thunk(void *ptr)
{
  return; 
}

#include <wchar.h>





wchar_t *
wcsdup_impl(const wchar_t *src)
{
  size_t len = wcslen(src);
  wchar_t *dst = (wchar_t*) malloc_impl((len + 1) * sizeof(wchar_t));
  if (dst)
    wcsncpy(dst, src, len + 1);
  return dst;
}
#endif 
