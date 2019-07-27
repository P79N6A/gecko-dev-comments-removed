



#include "Hal.h"
#include "HalLog.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
SetCurrentThreadPriority(ThreadPriority aPriority)
{
  HAL_LOG("FallbackThreadPriority - SetCurrentThreadPriority(%d)\n",
          ThreadPriorityToString(aPriority));
}

} 
} 
