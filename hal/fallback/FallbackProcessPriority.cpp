



#include "Hal.h"
#include "HalLog.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
SetProcessPriority(int aPid, ProcessPriority aPriority, uint32_t aLRU)
{
  HAL_LOG("FallbackProcessPriority - SetProcessPriority(%d, %s, %u)\n",
          aPid, ProcessPriorityToString(aPriority), aLRU);
}

} 
} 
