































#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_ATOMICWORD_COMPAT_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_ATOMICWORD_COMPAT_H_












#if !defined(GOOGLE_PROTOBUF_ARCH_64_BIT)

namespace google {
namespace protobuf {
namespace internal {

inline AtomicWord NoBarrier_CompareAndSwap(volatile AtomicWord* ptr,
                                           AtomicWord old_value,
                                           AtomicWord new_value) {
  return NoBarrier_CompareAndSwap(
      reinterpret_cast<volatile Atomic32*>(ptr), old_value, new_value);
}

inline AtomicWord NoBarrier_AtomicExchange(volatile AtomicWord* ptr,
                                           AtomicWord new_value) {
  return NoBarrier_AtomicExchange(
      reinterpret_cast<volatile Atomic32*>(ptr), new_value);
}

inline AtomicWord NoBarrier_AtomicIncrement(volatile AtomicWord* ptr,
                                            AtomicWord increment) {
  return NoBarrier_AtomicIncrement(
      reinterpret_cast<volatile Atomic32*>(ptr), increment);
}

inline AtomicWord Barrier_AtomicIncrement(volatile AtomicWord* ptr,
                                          AtomicWord increment) {
  return Barrier_AtomicIncrement(
      reinterpret_cast<volatile Atomic32*>(ptr), increment);
}

inline AtomicWord Acquire_CompareAndSwap(volatile AtomicWord* ptr,
                                         AtomicWord old_value,
                                         AtomicWord new_value) {
  return Acquire_CompareAndSwap(
      reinterpret_cast<volatile Atomic32*>(ptr), old_value, new_value);
}

inline AtomicWord Release_CompareAndSwap(volatile AtomicWord* ptr,
                                         AtomicWord old_value,
                                         AtomicWord new_value) {
  return Release_CompareAndSwap(
      reinterpret_cast<volatile Atomic32*>(ptr), old_value, new_value);
}

inline void NoBarrier_Store(volatile AtomicWord *ptr, AtomicWord value) {
  NoBarrier_Store(reinterpret_cast<volatile Atomic32*>(ptr), value);
}

inline void Acquire_Store(volatile AtomicWord* ptr, AtomicWord value) {
  return Acquire_Store(reinterpret_cast<volatile Atomic32*>(ptr), value);
}

inline void Release_Store(volatile AtomicWord* ptr, AtomicWord value) {
  return Release_Store(reinterpret_cast<volatile Atomic32*>(ptr), value);
}

inline AtomicWord NoBarrier_Load(volatile const AtomicWord *ptr) {
  return NoBarrier_Load(reinterpret_cast<volatile const Atomic32*>(ptr));
}

inline AtomicWord Acquire_Load(volatile const AtomicWord* ptr) {
  return Acquire_Load(reinterpret_cast<volatile const Atomic32*>(ptr));
}

inline AtomicWord Release_Load(volatile const AtomicWord* ptr) {
  return Release_Load(reinterpret_cast<volatile const Atomic32*>(ptr));
}

}   
}   
}   

#endif  

#endif  
