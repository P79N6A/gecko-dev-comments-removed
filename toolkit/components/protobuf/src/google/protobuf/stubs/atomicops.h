



















































#ifndef GOOGLE_PROTOBUF_ATOMICOPS_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_H_


#ifndef GOOGLE_PROTOBUF_NO_THREAD_SAFETY

#include <google/protobuf/stubs/platform_macros.h>

namespace google {
namespace protobuf {
namespace internal {

typedef int32 Atomic32;
#ifdef GOOGLE_PROTOBUF_ARCH_64_BIT


#if defined(__ILP32__) || defined(GOOGLE_PROTOBUF_OS_NACL) || defined(GOOGLE_PROTOBUF_ARCH_SPARC)



typedef int64 Atomic64;
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

#if defined(__MINGW32__) && defined(MemoryBarrier)
#undef MemoryBarrier
#endif
void MemoryBarrier();
void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value);
void Acquire_Store(volatile Atomic32* ptr, Atomic32 value);
void Release_Store(volatile Atomic32* ptr, Atomic32 value);

Atomic32 NoBarrier_Load(volatile const Atomic32* ptr);
Atomic32 Acquire_Load(volatile const Atomic32* ptr);
Atomic32 Release_Load(volatile const Atomic32* ptr);


#ifdef GOOGLE_PROTOBUF_ARCH_64_BIT
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
}  


#define GOOGLE_PROTOBUF_ATOMICOPS_ERROR \
#error "Atomic operations are not supported on your platform"


#if defined(THREAD_SANITIZER)
#include <google/protobuf/stubs/atomicops_internals_tsan.h>

#elif defined(_MSC_VER)
#if defined(GOOGLE_PROTOBUF_ARCH_IA32) || defined(GOOGLE_PROTOBUF_ARCH_X64)
#include <google/protobuf/stubs/atomicops_internals_x86_msvc.h>
#else
GOOGLE_PROTOBUF_ATOMICOPS_ERROR
#endif


#elif defined(GOOGLE_PROTOBUF_OS_SOLARIS)
#include <google/protobuf/stubs/atomicops_internals_solaris.h>


#elif defined(GOOGLE_PROTOBUF_OS_APPLE)
#include <google/protobuf/stubs/atomicops_internals_macosx.h>


#elif defined(__GNUC__)
#if defined(GOOGLE_PROTOBUF_ARCH_IA32) || defined(GOOGLE_PROTOBUF_ARCH_X64)
#include <google/protobuf/stubs/atomicops_internals_x86_gcc.h>
#elif defined(GOOGLE_PROTOBUF_ARCH_ARM) && defined(__linux__)
#include <google/protobuf/stubs/atomicops_internals_arm_gcc.h>
#elif defined(GOOGLE_PROTOBUF_ARCH_AARCH64)
#include <google/protobuf/stubs/atomicops_internals_arm64_gcc.h>
#elif defined(GOOGLE_PROTOBUF_ARCH_ARM_QNX)
#include <google/protobuf/stubs/atomicops_internals_arm_qnx.h>
#elif defined(GOOGLE_PROTOBUF_ARCH_MIPS) || defined(GOOGLE_PROTOBUF_ARCH_MIPS64)
#include <google/protobuf/stubs/atomicops_internals_mips_gcc.h>
#elif defined(__native_client__)
#include <google/protobuf/stubs/atomicops_internals_pnacl.h>
#elif (((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)) || (__GNUC__ > 4))
#include <google/protobuf/stubs/atomicops_internals_generic_gcc.h>
#elif defined(__clang__)
#if __has_extension(c_atomic)
#include <google/protobuf/stubs/atomicops_internals_generic_gcc.h>
#else
GOOGLE_PROTOBUF_ATOMICOPS_ERROR
#endif
#else
GOOGLE_PROTOBUF_ATOMICOPS_ERROR
#endif


#else
GOOGLE_PROTOBUF_ATOMICOPS_ERROR
#endif



#if defined(GOOGLE_PROTOBUF_OS_APPLE)
#include <google/protobuf/stubs/atomicops_internals_atomicword_compat.h>
#endif

#undef GOOGLE_PROTOBUF_ATOMICOPS_ERROR

#endif  

#endif  
