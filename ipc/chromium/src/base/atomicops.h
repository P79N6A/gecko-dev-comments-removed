


























#ifndef BASE_ATOMICOPS_H_
#define BASE_ATOMICOPS_H_

#include "base/basictypes.h"
#include "base/port.h"

namespace base {
namespace subtle {


#ifndef OS_WIN
#define __w64
#endif
typedef __w64 int32 Atomic32;
#ifdef CPU_ARCH_64_BITS
typedef int64 Atomic64;
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


#ifdef CPU_ARCH_64_BITS
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


#if defined(OS_WIN) && defined(COMPILER_MSVC) && defined(ARCH_CPU_X86_FAMILY)
#include "base/atomicops_internals_x86_msvc.h"
#elif defined(OS_MACOSX) && defined(ARCH_CPU_X86_FAMILY)
#include "base/atomicops_internals_x86_macosx.h"
#elif defined(COMPILER_GCC) && defined(ARCH_CPU_X86_FAMILY)
#include "base/atomicops_internals_x86_gcc.h"
#elif defined(COMPILER_GCC) && defined(ARCH_CPU_ARM_FAMILY)
#include "base/atomicops_internals_arm_gcc.h"
#else
#error "Atomic operations are not supported on your platform"
#endif

#endif  
