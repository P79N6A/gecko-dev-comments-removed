

































#ifndef _WIN32
# error You should only be including windows/port.cc in a windows environment!
#endif

#include "config.h"
#include <stdarg.h>    
#include <string.h>    
#include <assert.h>
#include <string>
#include <vector>
#include "port.h"

using std::string;
using std::vector;


int safe_vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  if (size == 0)        
    return -1;          
  str[size-1] = '\0';
  return _vsnprintf(str, size-1, format, ap);
}

int snprintf(char *str, size_t size, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  const int r = vsnprintf(str, size, format, ap);
  va_end(ap);
  return r;
}
