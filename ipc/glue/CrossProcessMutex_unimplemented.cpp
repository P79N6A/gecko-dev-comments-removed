




#include "CrossProcessMutex.h"

#include "nsDebug.h"

namespace mozilla {

CrossProcessMutex::CrossProcessMutex(const char*)
{
  NS_RUNTIMEABORT("Cross-process mutices not allowed on this platform.");
}

CrossProcessMutex::CrossProcessMutex(CrossProcessMutexHandle)
{
  NS_RUNTIMEABORT("Cross-process mutices not allowed on this platform.");
}

CrossProcessMutex::~CrossProcessMutex()
{
  NS_RUNTIMEABORT("Cross-process mutices not allowed on this platform - woah! We should've aborted by now!");
}

void
CrossProcessMutex::Lock()
{
  NS_RUNTIMEABORT("Cross-process mutices not allowed on this platform - woah! We should've aborted by now!");
}

void
CrossProcessMutex::Unlock()
{
  NS_RUNTIMEABORT("Cross-process mutices not allowed on this platform - woah! We should've aborted by now!");
}

CrossProcessMutexHandle
CrossProcessMutex::ShareToProcess(base::ProcessId aTargetPid)
{
  NS_RUNTIMEABORT("Cross-process mutices not allowed on this platform - woah! We should've aborted by now!");
  return 0;
}

}
