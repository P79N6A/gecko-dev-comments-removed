


























#ifndef BASE_ATOMICOPS_H_
#define BASE_ATOMICOPS_H_

#include "base/basictypes.h"
#include "base/port.h"

namespace base {
namespace subtle {


#ifndef OS_WIN
#define __w64
#endif
typedef __w64 int32_t Atomic32;
#ifdef ARCH_CPU_64_BITS
typedef int64_t Atomic64;
#endif



#ifdef OS_OPENBSD
#ifdef ARCH_CPU_64_BITS
typedef Atomic64 AtomicWord;
#else
typedef Atomic32 AtomicWord;
#endif 
#else
typedef intptr_t AtomicWord;
#endif 











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


#if defined(OS_WIN) && defined(ARCH_CPU_X86_FAMILY)
#include "base/atomicops_internals_x86_msvc.h"
#elif defined(OS_MACOSX) && defined(ARCH_CPU_X86_FAMILY)
#include "base/atomicops_internals_x86_macosx.h"
#elif defined(COMPILER_GCC) && defined(ARCH_CPU_X86_FAMILY)
#include "base/atomicops_internals_x86_gcc.h"
#elif defined(COMPILER_GCC) && defined(ARCH_CPU_ARM_FAMILY)
#include "base/atomicops_internals_arm_gcc.h"
#elif defined(COMPILER_GCC) && defined(ARCH_CPU_MIPS)
#include "base/atomicops_internals_mips_gcc.h"
#else
#include "base/atomicops_internals_mutex.h"
#endif

#endif  
