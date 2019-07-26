



#include "Hal.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
SetProcessPriority(int aPid,
                   ProcessPriority aPriority,
                   ProcessCPUPriority aCPUPriority,
                   uint32_t aBackgroundLRU)
{
  HAL_LOG(("FallbackProcessPriority - SetProcessPriority(%d, %s, %u)\n",
           aPid, ProcessPriorityToString(aPriority, aCPUPriority),
           aBackgroundLRU));
}

} 
} 
