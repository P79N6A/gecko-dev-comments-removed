







#ifndef BASE_ATOMICOPS_INTERNALS_ARM_GCC_H_
#define BASE_ATOMICOPS_INTERNALS_ARM_GCC_H_

#if defined(OS_QNX)
#include <sys/cpuinline.h>
#endif

namespace base {
namespace subtle {




























inline void MemoryBarrier() {
#if defined(OS_LINUX) || defined(OS_ANDROID)
  
  typedef void (*KernelMemoryBarrierFunc)();
  ((KernelMemoryBarrierFunc)0xffff0fa0)();
#elif defined(OS_QNX)
  __cpu_membarrier();
#else
#error MemoryBarrier() is not implemented on this platform.
#endif
}





#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || \
    defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || \
    defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || \
    defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || \
    defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__)

inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  Atomic32 prev_value;
  int reloop;
  do {
    
    
    
    
    
    
    __asm__ __volatile__("    ldrex %0, [%3]\n"
                         "    mov %1, #0\n"
                         "    cmp %0, %4\n"
#ifdef __thumb2__
                         "    it eq\n"
#endif
                         "    strexeq %1, %5, [%3]\n"
                         : "=&r"(prev_value), "=&r"(reloop), "+m"(*ptr)
                         : "r"(ptr), "r"(old_value), "r"(new_value)
                         : "cc", "memory");
  } while (reloop != 0);
  return prev_value;
}

inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  Atomic32 result = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  MemoryBarrier();
  return result;
}

inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  MemoryBarrier();
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  Atomic32 value;
  int reloop;
  do {
    
    
    
    
    
    
    __asm__ __volatile__("    ldrex %0, [%3]\n"
                         "    add %0, %0, %4\n"
                         "    strex %1, %0, [%3]\n"
                         : "=&r"(value), "=&r"(reloop), "+m"(*ptr)
                         : "r"(ptr), "r"(increment)
                         : "cc", "memory");
  } while (reloop);
  return value;
}

inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                        Atomic32 increment) {
  
  
  
  MemoryBarrier();
  Atomic32 result = NoBarrier_AtomicIncrement(ptr, increment);
  MemoryBarrier();
  return result;
}

inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  Atomic32 old_value;
  int reloop;
  do {
    
    
    __asm__ __volatile__("   ldrex %0, [%3]\n"
                         "   strex %1, %4, [%3]\n"
                         : "=&r"(old_value), "=&r"(reloop), "+m"(*ptr)
                         : "r"(ptr), "r"(new_value)
                         : "cc", "memory");
  } while (reloop != 0);
  return old_value;
}


#elif defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5T__) || \
      defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__)













namespace {

inline int LinuxKernelCmpxchg(Atomic32 old_value,
                              Atomic32 new_value,
                              volatile Atomic32* ptr) {
  typedef int (*KernelCmpxchgFunc)(Atomic32, Atomic32, volatile Atomic32*);
  return ((KernelCmpxchgFunc)0xffff0fc0)(old_value, new_value, ptr);
}

}  

inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  Atomic32 prev_value;
  for (;;) {
    prev_value = *ptr;
    if (prev_value != old_value)
      return prev_value;
    if (!LinuxKernelCmpxchg(old_value, new_value, ptr))
      return old_value;
  }
}

inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  Atomic32 old_value;
  do {
    old_value = *ptr;
  } while (LinuxKernelCmpxchg(old_value, new_value, ptr));
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
    if (!LinuxKernelCmpxchg(old_value, new_value, ptr)) {
      
      return new_value;
    }
    
  }
}

inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  Atomic32 prev_value;
  for (;;) {
    prev_value = *ptr;
    if (prev_value != old_value) {
      
      MemoryBarrier();
      return prev_value;
    }
    if (!LinuxKernelCmpxchg(old_value, new_value, ptr))
      return old_value;
  }
}

inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  
  
  
  
  
  
  
  
  
  
  return Acquire_CompareAndSwap(ptr, old_value, new_value);
}

#else
#  error "Your CPU's ARM architecture is not supported yet"
#endif




inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
}

inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
  MemoryBarrier();
}

inline void Release_Store(volatile Atomic32* ptr, Atomic32 value) {
  MemoryBarrier();
  *ptr = value;
}

inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr) { return *ptr; }

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

#endif  
