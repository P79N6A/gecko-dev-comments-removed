



#include "base/lazy_instance.h"

#include "base/at_exit.h"
#include "base/atomicops.h"
#include "base/basictypes.h"
#include "base/threading/platform_thread.h"
#include "base/third_party/dynamic_annotations/dynamic_annotations.h"

namespace base {
namespace internal {



bool NeedsLazyInstance(subtle::AtomicWord* state) {
  
  
  
  
  
  if (subtle::NoBarrier_CompareAndSwap(state, 0,
                                       kLazyInstanceStateCreating) == 0)
    
    return true;

  
  
  
  
  
  while (subtle::Acquire_Load(state) == kLazyInstanceStateCreating) {
    PlatformThread::YieldCurrentThread();
  }
  
  return false;
}

void CompleteLazyInstance(subtle::AtomicWord* state,
                          subtle::AtomicWord new_instance,
                          void* lazy_instance,
                          void (*dtor)(void*)) {
  
  ANNOTATE_HAPPENS_BEFORE(state);

  
  
  
  subtle::Release_Store(state, new_instance);

  
  if (dtor)
    AtExitManager::RegisterCallback(dtor, lazy_instance);
}

}  
}  
