





































#include "include/mozce_shunt.h"
#include <stdlib.h>

#ifdef MOZ_MEMORY
void * operator new(size_t _Size)
{
   void *p =  moz_malloc(_Size);
   return (p);
}

void operator delete(void * ptr)
{
  moz_free(ptr);  
}
void *operator new[](size_t size)
{
  void* p = moz_malloc(size);
  return (p);
}
void operator delete[](void *ptr)
{
  moz_free(ptr);
}

char*
mozce_strndup( const char *src, size_t len ) {
  char* dst = (char*)moz_malloc(len + 1);
  if(dst)
    strncpy(dst, src, len + 1);
  return dst;
}


char*
mozce_strdup(const char *src ) {
  size_t len = strlen(src);
  return mozce_strndup(src, len );
}

unsigned short* 
mozce_wcsndup( const unsigned short *src, size_t len ) {
  wchar_t* dst = (wchar_t*)moz_malloc(sizeof(wchar_t) * (len + 1));
  if(dst)
    wcsncpy(dst, src, len + 1);
  return dst;
}

unsigned short* 
mozce_wcsdup( const unsigned short *src ) {
  size_t len = wcslen(src);
  return mozce_wcsndup(src, len);
}
void* __cdecl malloc(size_t size) {
  return moz_malloc(size);
}
void* __cdecl valloc(size_t size) {
  return moz_valloc(size);
}
void* __cdecl calloc(size_t size, size_t num) {
  return moz_calloc(size, num);
}
void* __cdecl realloc(void* ptr, size_t size) {
  return moz_realloc(ptr, size);
}
void __cdecl free(void* ptr) {
  return moz_free(ptr);
}
#endif

