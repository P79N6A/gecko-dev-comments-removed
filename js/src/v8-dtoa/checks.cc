


























#include <stdarg.h>
#include <signal.h>

#include "v8.h"

static int fatal_error_handler_nesting_depth = 0;


extern "C" void V8_Fatal(const char* file, int line, const char* format, ...) {
  fflush(stdout);
  fflush(stderr);
  fatal_error_handler_nesting_depth++;
  
  
  
  
  
  
  if (fatal_error_handler_nesting_depth < 2) {
    fprintf(stderr, "\n\n#\n# Fatal error in %s, line %d\n# ", file, line);
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fprintf(stderr, "\n#\n\n");
  }

  i::OS::Abort();
}

