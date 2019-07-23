





































#include "include/mozce_shunt.h"
#include <stdlib.h>

#include "mozilla/mozalloc_macro_wrappers.h" 

#ifdef MOZ_MEMORY


const std::nothrow_t std::nothrow;

char*
_strndup(const char *src, size_t len) {
  char* dst = (char*)malloc(len + 1);
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
  wcsncpy(dst, src, len + 1);
  return dst;
}

wchar_t * 
_wcsdup(const wchar_t *src) {
  size_t len = wcslen(src);
  return _wcsndup(src, len);
}
#endif

