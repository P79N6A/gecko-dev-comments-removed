































#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_ARM_QNX_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_ARM_QNX_H_


#include <pthread.h>

namespace google {
namespace protobuf {
namespace internal {

inline Atomic32 QNXCmpxchg(Atomic32 old_value,
                           Atomic32 new_value,
                           volatile Atomic32* ptr) {
  return static_cast<Atomic32>(
      _smp_cmpxchg((volatile unsigned *)ptr,
                   (unsigned)old_value,
                   (unsigned)new_value));
}


inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  Atomic32 prev_value = *ptr;
  do {
    if (!QNXCmpxchg(old_value, new_value,
                    const_cast<Atomic32*>(ptr))) {
      return old_value;
    }
    prev_value = *ptr;
  } while (prev_value == old_value);
  return prev_value;
}

inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  Atomic32 old_value;
  do {
    old_value = *ptr;
  } while (QNXCmpxchg(old_value, new_value,
                      const_cast<Atomic32*>(ptr)));
  return old_value;
}

inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  return Barrier_AtomicIncrement(ptr, increment);
}

inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                        Atomic32 increment) {
  for (;;) {
    
    Atomic32 old_value = *ptr;
    Atomic32 new_value = old_value + increment;
    if (QNXCmpxchg(old_value, new_value,
                   const_cast<Atomic32*>(ptr)) == 0) {
      
      return new_value;
    }
    
  }
}

inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
}

inline void MemoryBarrier() {
  __sync_synchronize();
}

inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
  MemoryBarrier();
}

inline void Release_Store(volatile Atomic32* ptr, Atomic32 value) {
  MemoryBarrier();
  *ptr = value;
}

inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr) {
  return *ptr;
}

inline Atomic32 Acquire_Load(volatile const Atomic32* ptr) {
  Atomic32 value = *ptr;
  MemoryBarrier();
  return value;
}

inline Atomic32 Release_Load(volatile const Atomic32* ptr) {
  MemoryBarrier();
  return *ptr;
}

}  
}  
}  

#endif  
