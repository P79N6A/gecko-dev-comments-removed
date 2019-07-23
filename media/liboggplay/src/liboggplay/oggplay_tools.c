







































#include "oggplay_private.h"
#include <string.h>

ogg_int64_t
oggplay_sys_time_in_ms(void) {
#ifdef WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  return ((ogg_int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime) / 10000;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (ogg_int64_t)tv.tv_sec * 1000 + (ogg_int64_t)tv.tv_usec / 1000;
#endif
}

void
oggplay_millisleep(long ms) {
#ifdef WIN32
  Sleep(ms);
#elif defined(OS2)
  DosSleep(ms);
#else
  struct timespec ts = {0, (ogg_int64_t)ms * 1000000LL};
  nanosleep(&ts, NULL);
#endif
}


