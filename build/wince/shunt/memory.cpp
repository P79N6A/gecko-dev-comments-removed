





































#include "include/mozce_shunt.h"
#include <stdlib.h>

#ifdef MOZ_MEMORY
void * operator new(size_t _Size)
{
  void *p = malloc(_Size);
  return (p);
}

void operator delete(void * ptr)
{
  free(ptr);  
}
void *operator new[](size_t size)
{
  void* p = malloc(size);
  return (p);
}
void operator delete[](void *ptr)
{
  free(ptr);
}

char*
_strndup(const char *src, size_t len) {
  char* dst = (char*)malloc(len + 1);
  if(dst)
    strncpy(dst, src, len + 1);
  return dst;
}


char*
_strdup(const char *src) {
  size_t len = strlen(src);
  return _strndup(src, len );
}

wchar_t * 
_wcsndup(const wchar_t *src, size_t len) {
  wchar_t* dst = (wchar_t*)malloc(sizeof(wchar_t) * (len + 1));
  if(dst)
    wcsncpy(dst, src, len + 1);
  return dst;
}

wchar_t * 
_wcsdup(const wchar_t *src) {
  size_t len = wcslen(src);
  return _wcsndup(src, len);
}
#endif

