






#ifndef BASE_ATOMIC_REF_COUNT_H_
#define BASE_ATOMIC_REF_COUNT_H_

#include "base/atomicops.h"

namespace base {

typedef subtle::Atomic32 AtomicRefCount;


inline void AtomicRefCountIncN(volatile AtomicRefCount *ptr,
                               AtomicRefCount increment) {
  subtle::NoBarrier_AtomicIncrement(ptr, increment);
}





inline bool AtomicRefCountDecN(volatile AtomicRefCount *ptr,
                               AtomicRefCount decrement) {
  return subtle::Barrier_AtomicIncrement(ptr, -decrement) != 0;
}


inline void AtomicRefCountInc(volatile AtomicRefCount *ptr) {
  base::AtomicRefCountIncN(ptr, 1);
}




inline bool AtomicRefCountDec(volatile AtomicRefCount *ptr) {
  return base::AtomicRefCountDecN(ptr, 1);
}







inline bool AtomicRefCountIsOne(volatile AtomicRefCount *ptr) {
  return subtle::Acquire_Load(ptr) == 1;
}




inline bool AtomicRefCountIsZero(volatile AtomicRefCount *ptr) {
  return subtle::Acquire_Load(ptr) == 0;
}

}  

#endif  
