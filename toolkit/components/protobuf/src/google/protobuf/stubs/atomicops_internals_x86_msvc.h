































#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_X86_MSVC_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_X86_MSVC_H_

namespace google {
namespace protobuf {
namespace internal {

inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  return Barrier_AtomicIncrement(ptr, increment);
}

#if !(defined(_MSC_VER) && _MSC_VER >= 1400)
#error "We require at least vs2005 for MemoryBarrier"
#endif

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

inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value) {
  NoBarrier_AtomicExchange(ptr, value);
              
}

inline void Release_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;  
  
}

inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr) {
  return *ptr;
}

inline Atomic32 Acquire_Load(volatile const Atomic32* ptr) {
  Atomic32 value = *ptr;
  return value;
}

inline Atomic32 Release_Load(volatile const Atomic32* ptr) {
  MemoryBarrier();
  return *ptr;
}

#if defined(_WIN64)



inline Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr,
                                          Atomic64 increment) {
  return Barrier_AtomicIncrement(ptr, increment);
}

inline void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;
}

inline void Acquire_Store(volatile Atomic64* ptr, Atomic64 value) {
  NoBarrier_AtomicExchange(ptr, value);
              
}

inline void Release_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;  

  
  
  
  
  
  
}

inline Atomic64 NoBarrier_Load(volatile const Atomic64* ptr) {
  return *ptr;
}

inline Atomic64 Acquire_Load(volatile const Atomic64* ptr) {
  Atomic64 value = *ptr;
  return value;
}

inline Atomic64 Release_Load(volatile const Atomic64* ptr) {
  MemoryBarrier();
  return *ptr;
}

inline Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline Atomic64 Release_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

#endif  

}  
}  
}  

#endif  
