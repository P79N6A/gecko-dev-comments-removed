










#include "system_wrappers/interface/sleep.h"

#ifdef _WIN32

#include <windows.h>
#else

#include <time.h>
#endif

namespace webrtc {

void SleepMs(int msecs) {
#ifdef _WIN32
  Sleep(msecs);
#else
  struct timespec short_wait;
  struct timespec remainder;
  short_wait.tv_sec = msecs / 1000;
  short_wait.tv_nsec = (msecs % 1000) * 1000 * 1000;
  nanosleep(&short_wait, &remainder);
#endif
}

}  
