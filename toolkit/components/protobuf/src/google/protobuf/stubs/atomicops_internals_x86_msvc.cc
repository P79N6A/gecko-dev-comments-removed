

































#ifndef GOOGLE_PROTOBUF_NO_THREAD_SAFETY

#include <google/protobuf/stubs/atomicops.h>

#ifdef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_X86_MSVC_H_

#include <windows.h>

namespace google {
namespace protobuf {
namespace internal {

inline void MemoryBarrier() {
  
  ::MemoryBarrier();
}

Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                  Atomic32 old_value,
                                  Atomic32 new_value) {
  LONG result = InterlockedCompareExchange(
      reinterpret_cast<volatile LONG*>(ptr),
      static_cast<LONG>(new_value),
      static_cast<LONG>(old_value));
  return static_cast<Atomic32>(result);
}

Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                  Atomic32 new_value) {
  LONG result = InterlockedExchange(
      reinterpret_cast<volatile LONG*>(ptr),
      static_cast<LONG>(new_value));
  return static_cast<Atomic32>(result);
}

Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                 Atomic32 increment) {
  return InterlockedExchangeAdd(
      reinterpret_cast<volatile LONG*>(ptr),
      static_cast<LONG>(increment)) + increment;
}

#if defined(_WIN64)



Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
                                  Atomic64 old_value,
                                  Atomic64 new_value) {
  PVOID result = InterlockedCompareExchangePointer(
    reinterpret_cast<volatile PVOID*>(ptr),
    reinterpret_cast<PVOID>(new_value), reinterpret_cast<PVOID>(old_value));
  return reinterpret_cast<Atomic64>(result);
}

Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr,
                                  Atomic64 new_value) {
  PVOID result = InterlockedExchangePointer(
    reinterpret_cast<volatile PVOID*>(ptr),
    reinterpret_cast<PVOID>(new_value));
  return reinterpret_cast<Atomic64>(result);
}

Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr,
                                 Atomic64 increment) {
  return InterlockedExchangeAdd64(
      reinterpret_cast<volatile LONGLONG*>(ptr),
      static_cast<LONGLONG>(increment)) + increment;
}

#endif  

}  
}  
}  

#endif  
#endif  
