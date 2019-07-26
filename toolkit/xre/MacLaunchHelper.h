




#ifndef MacLaunchHelper_h_
#define MacLaunchHelper_h_

#include "mozilla/StandardInteger.h"

#include <unistd.h>

extern "C" {
  void LaunchChildMac(int aArgc, char** aArgv, uint32_t aRestartType = 0,
                      pid_t *pid = 0);
}

#endif
