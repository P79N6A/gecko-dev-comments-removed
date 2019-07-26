
































#ifndef alloc_hooks_h__
#define alloc_hooks_h__















#define mozilla_mozalloc_macro_wrappers_h

#include "mozilla/mozalloc.h"

extern void HunspellReportMemoryAllocation(void*);
extern void HunspellReportMemoryDeallocation(void*);

inline void* hunspell_malloc(size_t size)
{
  void* result = moz_malloc(size);
  HunspellReportMemoryAllocation(result);
  return result;
}
#define malloc(size) hunspell_malloc(size)

inline void* hunspell_calloc(size_t count, size_t size)
{
  void* result = moz_calloc(count, size);
  HunspellReportMemoryAllocation(result);
  return result;
}
#define calloc(count, size) hunspell_calloc(count, size)

inline void hunspell_free(void* ptr)
{
  HunspellReportMemoryDeallocation(ptr);
  moz_free(ptr);
}
#define free(ptr) hunspell_free(ptr)

inline void* hunspell_realloc(void* ptr, size_t size)
{
  HunspellReportMemoryDeallocation(ptr);
  void* result = moz_realloc(ptr, size);
  if (result) {
    HunspellReportMemoryAllocation(result);
  } else {
    
    HunspellReportMemoryAllocation(ptr);
  }
  return result;
}
#define realloc(ptr, size) hunspell_realloc(ptr, size)

inline char* hunspell_strdup(const char* str)
{
  char* result = moz_strdup(str);
  HunspellReportMemoryAllocation(result);
  return result;
}
#define strdup(str) hunspell_strdup(str)

#if defined(HAVE_STRNDUP)
inline char* hunspell_strndup(const char* str, size_t size)
{
  char* result = moz_strndup(str, size);
  HunspellReportMemoryAllocation(result);
  return result;
}
#define strndup(str, size) hunspell_strndup(str, size)
#endif

#endif
