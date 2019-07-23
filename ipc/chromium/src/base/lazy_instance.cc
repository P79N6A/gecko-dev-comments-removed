



#include "base/lazy_instance.h"

#include "base/at_exit.h"
#include "base/atomicops.h"
#include "base/basictypes.h"
#include "base/platform_thread.h"

namespace base {

void LazyInstanceHelper::EnsureInstance(void* instance,
                                        void (*ctor)(void*),
                                        void (*dtor)(void*)) {
  
  
  if (base::subtle::Acquire_CompareAndSwap(
          &state_, STATE_EMPTY, STATE_CREATING) == STATE_EMPTY) {
    
    ctor(instance);
    
    base::subtle::Release_Store(&state_, STATE_CREATED);
    
    base::AtExitManager::RegisterCallback(dtor, instance);
  } else {
    
    while (base::subtle::NoBarrier_Load(&state_) != STATE_CREATED)
      PlatformThread::YieldCurrentThread();
  }
}

}  
