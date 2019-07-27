





#include "SandboxUtil.h"
#include "SandboxLogging.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mozilla/Assertions.h"

namespace mozilla {

bool
IsSingleThreaded()
{
  
  
  
  
  
  struct stat sb;
  if (stat("/proc/self/task", &sb) < 0) {
    MOZ_DIAGNOSTIC_ASSERT(false, "Couldn't access /proc/self/task!");
    return false;
  }
  MOZ_DIAGNOSTIC_ASSERT(sb.st_nlink >= 3);
  return sb.st_nlink == 3;
}

} 
