



#ifndef BASE_STRING_UTIL_POSIX_H_
#define BASE_STRING_UTIL_POSIX_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "base/logging.h"

namespace base {



inline char* strdup(const char* str) {
  return ::strdup(str);
}

inline int strcasecmp(const char* string1, const char* string2) {
  return ::strcasecmp(string1, string2);
}

inline int strncasecmp(const char* string1, const char* string2, size_t count) {
  return ::strncasecmp(string1, string2, count);
}

inline int vsnprintf(char* buffer, size_t size,
                     const char* format, va_list arguments) {
  return ::vsnprintf(buffer, size, format, arguments);
}

inline int vswprintf(wchar_t* buffer, size_t size,
                     const wchar_t* format, va_list arguments) {
  DCHECK(IsWprintfFormatPortable(format));
  return ::vswprintf(buffer, size, format, arguments);
}

}  

#endif  
