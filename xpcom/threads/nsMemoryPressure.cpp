





#include "nsMemoryPressure.h"
#include "mozilla/Assertions.h"
#include "mozilla/Atomics.h"

#include "nsThreadUtils.h"

using namespace mozilla;

static Atomic<int32_t, Relaxed> sMemoryPressurePending;
static_assert(MemPressure_None == 0,
              "Bad static initialization with the default constructor.");

MemoryPressureState
NS_GetPendingMemoryPressure()
{
  int32_t value = sMemoryPressurePending.exchange(MemPressure_None);
  return MemoryPressureState(value);
}

void
NS_DispatchEventualMemoryPressure(MemoryPressureState aState)
{
  




  switch (aState) {
    case MemPressure_None:
      sMemoryPressurePending = MemPressure_None;
      break;
    case MemPressure_New:
      sMemoryPressurePending = MemPressure_New;
      break;
    case MemPressure_Ongoing:
      sMemoryPressurePending.compareExchange(MemPressure_None,
                                             MemPressure_Ongoing);
      break;
  }
}

nsresult
NS_DispatchMemoryPressure(MemoryPressureState aState)
{
  NS_DispatchEventualMemoryPressure(aState);
  nsCOMPtr<nsIRunnable> event = new nsRunnable;
  return NS_DispatchToMainThread(event);
}
