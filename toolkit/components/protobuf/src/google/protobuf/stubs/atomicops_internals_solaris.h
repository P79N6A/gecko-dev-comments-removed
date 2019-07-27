






























#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_SPARC_GCC_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_SPARC_GCC_H_

#include <atomic.h>

namespace google {
namespace protobuf {
namespace internal {

inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  return (Atomic32)atomic_cas_32((volatile uint32_t*)ptr, (uint32_t)old_value, (uint32_t)new_value);
}

inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  return (Atomic32)atomic_swap_32((volatile uint32_t*)ptr, (uint32_t)new_value);
}

inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  return (Atomic32)atomic_add_32_nv((volatile uint32_t*)ptr, (uint32_t)increment);
}

inline void MemoryBarrier(void) {
	membar_producer();
	membar_consumer();
}

inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                        Atomic32 increment) {
  MemoryBarrier();
  Atomic32 ret = NoBarrier_AtomicIncrement(ptr, increment);
  MemoryBarrier();

  return ret;
}

inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  Atomic32 ret = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  MemoryBarrier();

  return ret;
}

inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  MemoryBarrier();
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
}

inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
  membar_producer();
}

inline void Release_Store(volatile Atomic32* ptr, Atomic32 value) {
  membar_consumer();
  *ptr = value;
}

inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr) {
  return *ptr;
}

inline Atomic32 Acquire_Load(volatile const Atomic32* ptr) {
  Atomic32 val = *ptr;
  membar_consumer();
  return val;
}

inline Atomic32 Release_Load(volatile const Atomic32* ptr) {
  membar_producer();
  return *ptr;
}

#ifdef GOOGLE_PROTOBUF_ARCH_64_BIT
inline Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
                                         Atomic64 old_value,
                                         Atomic64 new_value) {
  return atomic_cas_64((volatile uint64_t*)ptr, (uint64_t)old_value, (uint64_t)new_value);
}

inline Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr, Atomic64 new_value) {
  return atomic_swap_64((volatile uint64_t*)ptr, (uint64_t)new_value);
}

inline Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr, Atomic64 increment) {
  return atomic_add_64_nv((volatile uint64_t*)ptr, increment);
}

inline Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr, Atomic64 increment) {
  MemoryBarrier();
  Atomic64 ret = atomic_add_64_nv((volatile uint64_t*)ptr, increment);
  MemoryBarrier();
  return ret;
}

inline Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  Atomic64 ret = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  MemoryBarrier();
  return ret;
}

inline Atomic64 Release_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  MemoryBarrier();
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;
}

inline void Acquire_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;
  membar_producer();
}

inline void Release_Store(volatile Atomic64* ptr, Atomic64 value) {
  membar_consumer();
  *ptr = value;
}

inline Atomic64 NoBarrier_Load(volatile const Atomic64* ptr) {
  return *ptr;
}

inline Atomic64 Acquire_Load(volatile const Atomic64* ptr) {
  Atomic64 ret = *ptr;
  membar_consumer();
  return ret;
}

inline Atomic64 Release_Load(volatile const Atomic64* ptr) {
  membar_producer();
  return *ptr;
}
#endif

}  
}  
}  

#endif  

