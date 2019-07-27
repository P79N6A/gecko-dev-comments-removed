



#include "base/memory/singleton.h"
#include "base/threading/platform_thread.h"

namespace base {
namespace internal {

subtle::AtomicWord WaitForInstance(subtle::AtomicWord* instance) {
  
  
  
  
  
  
  
  subtle::AtomicWord value;
  while (true) {
    
    
    
    value = subtle::Acquire_Load(instance);
    if (value != kBeingCreatedMarker)
      break;
    PlatformThread::YieldCurrentThread();
  }
  return value;
}

}  
}  

