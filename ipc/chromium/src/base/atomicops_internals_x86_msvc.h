





#ifndef BASE_ATOMICOPS_INTERNALS_X86_MSVC_H_
#define BASE_ATOMICOPS_INTERNALS_X86_MSVC_H_

#include <windows.h>

namespace base {
namespace subtle {

inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  LONG result = InterlockedCompareExchange(
      reinterpret_cast<volatile LONG*>(ptr),
      static_cast<LONG>(new_value),
      static_cast<LONG>(old_value));
  return static_cast<Atomic32>(result);
}

inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  LONG result = InterlockedExchange(
      reinterpret_cast<volatile LONG*>(ptr),
      static_cast<LONG>(new_value));
  return static_cast<Atomic32>(result);
}

inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                        Atomic32 increment) {
  return InterlockedExchangeAdd(
      reinterpret_cast<volatile LONG*>(ptr),
      static_cast<LONG>(increment)) + increment;
}

inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  return Barrier_AtomicIncrement(ptr, increment);
}

#if !(defined(_MSC_VER) && _MSC_VER >= 1400)
#error "We require at least vs2005 for MemoryBarrier"
#endif
inline void MemoryBarrier() {
  
  ::MemoryBarrier();
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



COMPILE_ASSERT(sizeof(Atomic64) == sizeof(PVOID), atomic_word_is_atomic);

inline Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
                                         Atomic64 old_value,
                                         Atomic64 new_value) {
  PVOID result = InterlockedCompareExchangePointer(
    reinterpret_cast<volatile PVOID*>(ptr),
    reinterpret_cast<PVOID>(new_value), reinterpret_cast<PVOID>(old_value));
  return reinterpret_cast<Atomic64>(result);
}

inline Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr,
                                         Atomic64 new_value) {
  PVOID result = InterlockedExchangePointer(
    reinterpret_cast<volatile PVOID*>(ptr),
    reinterpret_cast<PVOID>(new_value));
  return reinterpret_cast<Atomic64>(result);
}

inline Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr,
                                        Atomic64 increment) {
  return InterlockedExchangeAdd64(
      reinterpret_cast<volatile LONGLONG*>(ptr),
      static_cast<LONGLONG>(increment)) + increment;
}

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

#endif  

}  
}  

#endif  
