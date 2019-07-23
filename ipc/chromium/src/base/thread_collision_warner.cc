



#include "base/thread_collision_warner.h"

#include "base/logging.h"
#include "base/platform_thread.h"

namespace base {

void DCheckAsserter::warn() {
  NOTREACHED() << "Thread Collision";
}

static subtle::Atomic32 CurrentThread() {
  const PlatformThreadId current_thread_id = PlatformThread::CurrentId();
  
  
  
  const subtle::Atomic32 atomic_thread_id =
      static_cast<subtle::Atomic32>(current_thread_id);

  return atomic_thread_id;
}

void ThreadCollisionWarner::EnterSelf() {
  
  
  
  subtle::Atomic32 current_thread_id = CurrentThread();

  int previous_value = subtle::NoBarrier_CompareAndSwap(&valid_thread_id_,
                                                        0,
                                                        current_thread_id);
  if (previous_value != 0 && previous_value != current_thread_id) {
    
    
    asserter_->warn();
  }

  subtle::NoBarrier_AtomicIncrement(&counter_, 1);
}

void ThreadCollisionWarner::Enter() {
  subtle::Atomic32 current_thread_id = CurrentThread();

  if (subtle::NoBarrier_CompareAndSwap(&valid_thread_id_,
                                       0,
                                       current_thread_id) != 0) {
    
    asserter_->warn();
  }

  subtle::NoBarrier_AtomicIncrement(&counter_, 1);
}

void ThreadCollisionWarner::Leave() {
  if (subtle::Barrier_AtomicIncrement(&counter_, -1) == 0) {
    subtle::NoBarrier_Store(&valid_thread_id_, 0);
  }
}

}  
