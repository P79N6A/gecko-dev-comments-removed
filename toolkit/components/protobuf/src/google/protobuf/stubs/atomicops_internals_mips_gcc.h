































#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_MIPS_GCC_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_MIPS_GCC_H_

#define ATOMICOPS_COMPILER_BARRIER() __asm__ __volatile__("" : : : "memory")

namespace google {
namespace protobuf {
namespace internal {











inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  Atomic32 prev, tmp;
  __asm__ __volatile__(".set push\n"
                       ".set noreorder\n"
                       "1:\n"
                       "ll %0, %5\n"  
                       "bne %0, %3, 2f\n"  
                       "move %2, %4\n"  
                       "sc %2, %1\n"  
                       "beqz %2, 1b\n"  
                       "nop\n"  
                       "2:\n"
                       ".set pop\n"
                       : "=&r" (prev), "=m" (*ptr), "=&r" (tmp)
                       : "Ir" (old_value), "r" (new_value), "m" (*ptr)
                       : "memory");
  return prev;
}



inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  Atomic32 temp, old;
  __asm__ __volatile__(".set push\n"
                       ".set noreorder\n"
                       "1:\n"
                       "ll %1, %4\n"  
                       "move %0, %3\n"  
                       "sc %0, %2\n"  
                       "beqz %0, 1b\n"  
                       "nop\n"  
                       ".set pop\n"
                       : "=&r" (temp), "=&r" (old), "=m" (*ptr)
                       : "r" (new_value), "m" (*ptr)
                       : "memory");

  return old;
}



inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  Atomic32 temp, temp2;

  __asm__ __volatile__(".set push\n"
                       ".set noreorder\n"
                       "1:\n"
                       "ll %0, %4\n"  
                       "addu %1, %0, %3\n"  
                       "sc %1, %2\n"  
                       "beqz %1, 1b\n"  
                       "addu %1, %0, %3\n"  
                       ".set pop\n"
                       : "=&r" (temp), "=&r" (temp2), "=m" (*ptr)
                       : "Ir" (increment), "m" (*ptr)
                       : "memory");
  
  return temp2;
}

inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                        Atomic32 increment) {
  ATOMICOPS_COMPILER_BARRIER();
  Atomic32 res = NoBarrier_AtomicIncrement(ptr, increment);
  ATOMICOPS_COMPILER_BARRIER();
  return res;
}







inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  ATOMICOPS_COMPILER_BARRIER();
  Atomic32 res = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  ATOMICOPS_COMPILER_BARRIER();
  return res;
}

inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  ATOMICOPS_COMPILER_BARRIER();
  Atomic32 res = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  ATOMICOPS_COMPILER_BARRIER();
  return res;
}

inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
}

inline void MemoryBarrier() {
  __asm__ __volatile__("sync" : : : "memory");
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

#if defined(__LP64__)


inline Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
                                         Atomic64 old_value,
                                         Atomic64 new_value) {
  Atomic64 prev, tmp;
  __asm__ __volatile__(".set push\n"
                       ".set noreorder\n"
                       "1:\n"
                       "lld %0, %5\n"  
                       "bne %0, %3, 2f\n"  
                       "move %2, %4\n"  
                       "scd %2, %1\n"  
                       "beqz %2, 1b\n"  
                       "nop\n"  
                       "2:\n"
                       ".set pop\n"
                       : "=&r" (prev), "=m" (*ptr), "=&r" (tmp)
                       : "Ir" (old_value), "r" (new_value), "m" (*ptr)
                       : "memory");
  return prev;
}



inline Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr,
                                         Atomic64 new_value) {
  Atomic64 temp, old;
  __asm__ __volatile__(".set push\n"
                       ".set noreorder\n"
                       "1:\n"
                       "lld %1, %4\n"  
                       "move %0, %3\n"  
                       "scd %0, %2\n"  
                       "beqz %0, 1b\n"  
                       "nop\n"  
                       ".set pop\n"
                       : "=&r" (temp), "=&r" (old), "=m" (*ptr)
                       : "r" (new_value), "m" (*ptr)
                       : "memory");

  return old;
}



inline Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr,
                                          Atomic64 increment) {
  Atomic64 temp, temp2;

  __asm__ __volatile__(".set push\n"
                       ".set noreorder\n"
                       "1:\n"
                       "lld %0, %4\n"  
                       "daddu %1, %0, %3\n"  
                       "scd %1, %2\n"  
                       "beqz %1, 1b\n"  
                       "daddu %1, %0, %3\n"  
                       ".set pop\n"
                       : "=&r" (temp), "=&r" (temp2), "=m" (*ptr)
                       : "Ir" (increment), "m" (*ptr)
                       : "memory");
  
  return temp2;
}

inline Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr,
                                        Atomic64 increment) {
  MemoryBarrier();
  Atomic64 res = NoBarrier_AtomicIncrement(ptr, increment);
  MemoryBarrier();
  return res;
}







inline Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  Atomic64 res = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  MemoryBarrier();
  return res;
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
  MemoryBarrier();
}

inline void Release_Store(volatile Atomic64* ptr, Atomic64 value) {
  MemoryBarrier();
  *ptr = value;
}

inline Atomic64 NoBarrier_Load(volatile const Atomic64* ptr) {
  return *ptr;
}

inline Atomic64 Acquire_Load(volatile const Atomic64* ptr) {
  Atomic64 value = *ptr;
  MemoryBarrier();
  return value;
}

inline Atomic64 Release_Load(volatile const Atomic64* ptr) {
  MemoryBarrier();
  return *ptr;
}
#endif

}  
}  
}  

#undef ATOMICOPS_COMPILER_BARRIER

#endif  
