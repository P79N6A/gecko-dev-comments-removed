
































#include <execinfo.h>
#include <string.h>
#include "stacktrace.h"

_START_GOOGLE_NAMESPACE_


int GetStackTrace(void** result, int max_depth, int skip_count) {
  static const int kStackLength = 64;
  void * stack[kStackLength];
  int size;

  size = backtrace(stack, kStackLength);
  skip_count++;  
  int result_count = size - skip_count;
  if (result_count < 0)
    result_count = 0;
  if (result_count > max_depth)
    result_count = max_depth;
  for (int i = 0; i < result_count; i++)
    result[i] = stack[i + skip_count];

  return result_count;
}

_END_GOOGLE_NAMESPACE_
