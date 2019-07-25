



#include "Hal.h"

using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

void
SetProcessPriority(int aPid, ProcessPriority aPriority)
{
  HAL_LOG(("FallbackProcessPriority - SetProcessPriority(%d, %d)\n", aPid, aPriority));
}

} 
} 
