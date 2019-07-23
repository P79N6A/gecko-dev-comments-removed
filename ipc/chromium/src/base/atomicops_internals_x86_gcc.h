





#ifndef BASE_ATOMICOPS_INTERNALS_X86_GCC_H_
#define BASE_ATOMICOPS_INTERNALS_X86_GCC_H_





struct AtomicOps_x86CPUFeatureStruct {
  bool has_amd_lock_mb_bug; 
                            
  bool has_sse2;            
};
extern struct AtomicOps_x86CPUFeatureStruct AtomicOps_Internalx86CPUFeatures;

#define ATOMICOPS_COMPILER_BARRIER() __asm__ __volatile__("" : : : "memory")

namespace base {
namespace subtle {



inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  Atomic32 prev;
  __asm__ __volatile__("lock; cmpxchgl %1,%2"
                       : "=a" (prev)
                       : "q" (new_value), "m" (*ptr), "0" (old_value)
                       : "memory");
  return prev;
}

inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  __asm__ __volatile__("xchgl %1,%0"  
                       : "=r" (new_value)
                       : "m" (*ptr), "0" (new_value)
                       : "memory");
  return new_value;  
}

inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  Atomic32 temp = increment;
  __asm__ __volatile__("lock; xaddl %0,%1"
                       : "+r" (temp), "+m" (*ptr)
                       : : "memory");
  
  return temp + increment;
}

inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                        Atomic32 increment) {
  Atomic32 temp = increment;
  __asm__ __volatile__("lock; xaddl %0,%1"
                       : "+r" (temp), "+m" (*ptr)
                       : : "memory");
  
  if (AtomicOps_Internalx86CPUFeatures.has_amd_lock_mb_bug) {
    __asm__ __volatile__("lfence" : : : "memory");
  }
  return temp + increment;
}

inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  Atomic32 x = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  if (AtomicOps_Internalx86CPUFeatures.has_amd_lock_mb_bug) {
    __asm__ __volatile__("lfence" : : : "memory");
  }
  return x;
}

inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
}

#if defined(__x86_64__)



inline void MemoryBarrier() {
  __asm__ __volatile__("mfence" : : : "memory");
}

inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
  MemoryBarrier();
}

#else

inline void MemoryBarrier() {
  if (AtomicOps_Internalx86CPUFeatures.has_sse2) {
    __asm__ __volatile__("mfence" : : : "memory");
  } else { 
    Atomic32 x = 0;
    NoBarrier_AtomicExchange(&x, 0);  
  }
}

inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value) {
  if (AtomicOps_Internalx86CPUFeatures.has_sse2) {
    *ptr = value;
    __asm__ __volatile__("mfence" : : : "memory");
  } else {
    NoBarrier_AtomicExchange(ptr, value);
                          
  }
}
#endif

inline void Release_Store(volatile Atomic32* ptr, Atomic32 value) {
  ATOMICOPS_COMPILER_BARRIER();
  *ptr = value; 
  
}

inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr) {
  return *ptr;
}

inline Atomic32 Acquire_Load(volatile const Atomic32* ptr) {
  Atomic32 value = *ptr; 
  
  ATOMICOPS_COMPILER_BARRIER();
  return value;
}

inline Atomic32 Release_Load(volatile const Atomic32* ptr) {
  MemoryBarrier();
  return *ptr;
}

#if defined(__x86_64__)



inline Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
                                         Atomic64 old_value,
                                         Atomic64 new_value) {
  Atomic64 prev;
  __asm__ __volatile__("lock; cmpxchgq %1,%2"
                       : "=a" (prev)
                       : "q" (new_value), "m" (*ptr), "0" (old_value)
                       : "memory");
  return prev;
}

inline Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr,
                                         Atomic64 new_value) {
  __asm__ __volatile__("xchgq %1,%0"  
                       : "=r" (new_value)
                       : "m" (*ptr), "0" (new_value)
                       : "memory");
  return new_value;  
}

inline Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr,
                                          Atomic64 increment) {
  Atomic64 temp = increment;
  __asm__ __volatile__("lock; xaddq %0,%1"
                       : "+r" (temp), "+m" (*ptr)
                       : : "memory");
  
  return temp + increment;
}

inline Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr,
                                        Atomic64 increment) {
  Atomic64 temp = increment;
  __asm__ __volatile__("lock; xaddq %0,%1"
                       : "+r" (temp), "+m" (*ptr)
                       : : "memory");
  
  if (AtomicOps_Internalx86CPUFeatures.has_amd_lock_mb_bug) {
    __asm__ __volatile__("lfence" : : : "memory");
  }
  return temp + increment;
}

inline void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;
}

#if defined(CHROMIUM_MOZILLA_BUILD)
inline Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  Atomic64 x = NoBarrier_CompareAndSwap(ptr, old_value, new_value);
  
  if (AtomicOps_Internalx86CPUFeatures.has_amd_lock_mb_bug) {
    __asm__ __volatile__("lfence" : : : "memory");
  }
  return x;
}
#endif

inline void Acquire_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;
  MemoryBarrier();
}

inline void Release_Store(volatile Atomic64* ptr, Atomic64 value) {
  ATOMICOPS_COMPILER_BARRIER();

  *ptr = value; 
                
                

  
  
  
  
  
  
  
  
  
  
  
  
}

inline Atomic64 NoBarrier_Load(volatile const Atomic64* ptr) {
  return *ptr;
}

inline Atomic64 Acquire_Load(volatile const Atomic64* ptr) {
  Atomic64 value = *ptr; 
                         
                         
  ATOMICOPS_COMPILER_BARRIER();
  return value;
}

inline Atomic64 Release_Load(volatile const Atomic64* ptr) {
  MemoryBarrier();
  return *ptr;
}
#endif  

} 
} 

#undef ATOMICOPS_COMPILER_BARRIER

#endif  
