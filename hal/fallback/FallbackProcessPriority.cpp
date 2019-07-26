



#include "Hal.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
SetProcessPriority(int aPid,
                   ProcessPriority aPriority,
                   ProcessCPUPriority aCPUPriority)
{
  HAL_LOG(("FallbackProcessPriority - SetProcessPriority(%d, %s)\n",
           aPid, ProcessPriorityToString(aPriority, aCPUPriority)));
}

} 
} 
