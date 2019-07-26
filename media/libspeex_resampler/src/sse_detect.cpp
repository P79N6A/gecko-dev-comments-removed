




#include "mozilla/SSE.h"
#include "sse_detect.h"

int moz_has_sse2() {
  return mozilla::supports_sse2() ? 1 : 0;
}

int moz_has_sse() {
  return mozilla::supports_sse() ? 1 : 0;
}
