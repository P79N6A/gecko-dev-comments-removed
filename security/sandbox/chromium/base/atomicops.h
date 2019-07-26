


























#ifndef BASE_ATOMICOPS_H_
#define BASE_ATOMICOPS_H_

#include "base/basictypes.h"
#include "build/build_config.h"

#if defined(OS_WIN) && defined(ARCH_CPU_64_BITS)





#undef MemoryBarrier
#endif

namespace base {
namespace subtle {

typedef int32 Atomic32;
#ifdef ARCH_CPU_64_BITS


#if defined(__ILP32__) || defined(OS_NACL)


typedef int64_t Atomic64;
#else
typedef intptr_t Atomic64;
#endif
#endif



typedef intptr_t AtomicWord;











Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                  Atomic32 old_value,
                                  Atomic32 new_value);



Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr, Atomic32 new_value);



Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr, Atomic32 increment);

Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                 Atomic32 increment);










Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                Atomic32 old_value,
                                Atomic32 new_value);
Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                Atomic32 old_value,
                                Atomic32 new_value);

void MemoryBarrier();
void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value);
void Acquire_Store(volatile Atomic32* ptr, Atomic32 value);
void Release_Store(volatile Atomic32* ptr, Atomic32 value);

Atomic32 NoBarrier_Load(volatile const Atomic32* ptr);
Atomic32 Acquire_Load(volatile const Atomic32* ptr);
Atomic32 Release_Load(volatile const Atomic32* ptr);


#ifdef ARCH_CPU_64_BITS
Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
                                  Atomic64 old_value,
                                  Atomic64 new_value);
Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr, Atomic64 new_value);
Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr, Atomic64 increment);
Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr, Atomic64 increment);

Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
                                Atomic64 old_value,
                                Atomic64 new_value);
Atomic64 Release_CompareAndSwap(volatile Atomic64* ptr,
                                Atomic64 old_value,
                                Atomic64 new_value);
void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value);
void Acquire_Store(volatile Atomic64* ptr, Atomic64 value);
void Release_Store(volatile Atomic64* ptr, Atomic64 value);
Atomic64 NoBarrier_Load(volatile const Atomic64* ptr);
Atomic64 Acquire_Load(volatile const Atomic64* ptr);
Atomic64 Release_Load(volatile const Atomic64* ptr);
#endif  

}  
}  


#if defined(THREAD_SANITIZER)
#include "base/atomicops_internals_tsan.h"
#elif defined(OS_WIN) && defined(COMPILER_MSVC) && defined(ARCH_CPU_X86_FAMILY)
#include "base/atomicops_internals_x86_msvc.h"
#elif defined(OS_MACOSX)
#include "base/atomicops_internals_mac.h"
#elif defined(OS_NACL)
#include "base/atomicops_internals_gcc.h"
#elif defined(COMPILER_GCC) && defined(ARCH_CPU_ARM_FAMILY)
#include "base/atomicops_internals_arm_gcc.h"
#elif defined(COMPILER_GCC) && defined(ARCH_CPU_X86_FAMILY)
#include "base/atomicops_internals_x86_gcc.h"
#elif defined(COMPILER_GCC) && defined(ARCH_CPU_MIPS_FAMILY)
#include "base/atomicops_internals_mips_gcc.h"
#else
#error "Atomic operations are not supported on your platform"
#endif



#if defined(OS_MACOSX) || defined(OS_OPENBSD)
#include "base/atomicops_internals_atomicword_compat.h"
#endif

#endif  
