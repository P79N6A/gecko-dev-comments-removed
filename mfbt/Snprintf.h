








#ifndef mozilla_Snprintf_h_
#define mozilla_Snprintf_h_

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>





#if defined(_MSC_VER) && _MSC_VER < 1900
#include "mozilla/Attributes.h"
MOZ_ALWAYS_INLINE int snprintf(char* buffer, size_t n, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int result = vsnprintf(buffer, n, format, args);
  va_end(args);
  buffer[n - 1] = '\0';
  return result;
}
#endif




#ifdef __cplusplus
template <size_t N>
int snprintf_literal(char (&buffer)[N], const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int result = vsnprintf(buffer, N, format, args);
  va_end(args);
  buffer[N - 1] = '\0';
  return result;
}
#endif

#endif  
