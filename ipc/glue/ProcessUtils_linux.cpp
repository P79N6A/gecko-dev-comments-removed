



#include "ProcessUtils.h"

#include "nsString.h"

#include <sys/prctl.h>

namespace mozilla {
namespace ipc {

void SetThisProcessName(const char *aName)
{
  prctl(PR_SET_NAME, (unsigned long)aName, 0uL, 0uL, 0uL);
}

} 
} 
