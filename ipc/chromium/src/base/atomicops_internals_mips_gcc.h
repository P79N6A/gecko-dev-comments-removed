




























#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_MIPS_GCC_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_MIPS_GCC_H_

#define ATOMICOPS_COMPILER_BARRIER() __asm__ __volatile__("" : : : "memory")

namespace base {
namespace subtle {











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
                       "ll %1, %2\n"  
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
                       "ll %0, %2\n"  
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

}  
}  

#undef ATOMICOPS_COMPILER_BARRIER

#endif  
