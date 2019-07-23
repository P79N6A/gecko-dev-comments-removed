



#include "base/debug_util.h"

#include <signal.h>

#include "base/basictypes.h"

static void ExitSignalHandler(int sig) {
  exit(128 + sig);
}


void DebugUtil::DisableOSCrashDumps() {
  int signals_to_intercept[] ={SIGINT,
                               SIGHUP,
                               SIGTERM,
                               SIGABRT,
                               SIGILL,
                               SIGTRAP,
                               SIGEMT,
                               SIGFPE,
                               SIGBUS,
                               SIGSEGV,
                               SIGSYS,
                               SIGPIPE,
                               SIGXCPU,
                               SIGXFSZ};
  
  for (size_t i = 0; i < arraysize(signals_to_intercept); ++i) {
    signal(signals_to_intercept[i], ExitSignalHandler);
  }
}
