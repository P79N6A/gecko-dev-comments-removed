




































#include <google/protobuf/stubs/once.h>

#ifndef GOOGLE_PROTOBUF_NO_THREAD_SAFETY

#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#endif

#include <google/protobuf/stubs/atomicops.h>

namespace google {
namespace protobuf {

namespace {

void SchedYield() {
#ifdef _WIN32
  Sleep(0);
#else  
  sched_yield();
#endif
}

}  

void GoogleOnceInitImpl(ProtobufOnceType* once, Closure* closure) {
  internal::AtomicWord state = internal::Acquire_Load(once);
  
  if (state == ONCE_STATE_DONE) {
    return;
  }
  
  
  
  
  
  
  
  state = internal::Acquire_CompareAndSwap(
      once, ONCE_STATE_UNINITIALIZED, ONCE_STATE_EXECUTING_CLOSURE);
  if (state == ONCE_STATE_UNINITIALIZED) {
    
    
    closure->Run();
    internal::Release_Store(once, ONCE_STATE_DONE);
  } else {
    
    
    while (state == ONCE_STATE_EXECUTING_CLOSURE) {
      
      SchedYield();
      state = internal::Acquire_Load(once);
    }
  }
}

}  
}  

#endif  
