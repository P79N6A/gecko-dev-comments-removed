


























#ifndef V8_GLOBALS_H_
#define V8_GLOBALS_H_

namespace v8 {
namespace internal {





#if defined(_M_X64) || defined(__x86_64__)
#define V8_HOST_ARCH_X64 1
#define V8_HOST_ARCH_64_BIT 1
#define V8_HOST_CAN_READ_UNALIGNED 1
#elif defined(_M_IX86) || defined(__i386__)
#define V8_HOST_ARCH_IA32 1
#define V8_HOST_ARCH_32_BIT 1
#define V8_HOST_CAN_READ_UNALIGNED 1
#elif defined(__ARMEL__)
#define V8_HOST_ARCH_ARM 1
#define V8_HOST_ARCH_32_BIT 1



#if CAN_USE_UNALIGNED_ACCESSES
#define V8_HOST_CAN_READ_UNALIGNED 1
#endif
#elif defined(_MIPS_ARCH_MIPS32R2)
#define V8_HOST_ARCH_MIPS 1
#define V8_HOST_ARCH_32_BIT 1
#else
#error Host architecture was not detected as supported by v8
#endif




#if !defined(V8_TARGET_ARCH_X64) && !defined(V8_TARGET_ARCH_IA32) && \
    !defined(V8_TARGET_ARCH_ARM) && !defined(V8_TARGET_ARCH_MIPS)
#if defined(_M_X64) || defined(__x86_64__)
#define V8_TARGET_ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define V8_TARGET_ARCH_IA32 1
#elif defined(__ARMEL__)
#define V8_TARGET_ARCH_ARM 1
#elif defined(_MIPS_ARCH_MIPS32R2)
#define V8_TARGET_ARCH_MIPS 1
#else
#error Target architecture was not detected as supported by v8
#endif
#endif


#if defined(V8_TARGET_ARCH_IA32) && !defined(V8_HOST_ARCH_IA32)
#error Target architecture ia32 is only supported on ia32 host
#endif
#if defined(V8_TARGET_ARCH_X64) && !defined(V8_HOST_ARCH_X64)
#error Target architecture x64 is only supported on x64 host
#endif
#if (defined(V8_TARGET_ARCH_ARM) && \
    !(defined(V8_HOST_ARCH_IA32) || defined(V8_HOST_ARCH_ARM)))
#error Target architecture arm is only supported on arm and ia32 host
#endif
#if (defined(V8_TARGET_ARCH_MIPS) && \
    !(defined(V8_HOST_ARCH_IA32) || defined(V8_HOST_ARCH_MIPS)))
#error Target architecture mips is only supported on mips and ia32 host
#endif


#if defined(V8_TARGET_ARCH_X64) || defined(V8_TARGET_ARCH_IA32)
#define V8_TARGET_CAN_READ_UNALIGNED 1
#elif V8_TARGET_ARCH_ARM



#if CAN_USE_UNALIGNED_ACCESSES
#define V8_TARGET_CAN_READ_UNALIGNED 1
#endif
#elif V8_TARGET_ARCH_MIPS
#else
#error Target architecture is not supported by v8
#endif




#define V8_2PART_UINT64_C(a, b) (((static_cast<uint64_t>(a) << 32) + 0x##b##u))




const int kCharSize     = sizeof(char);      








template <typename T>
static inline void USE(T) { }




#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)








#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

} }  

#endif  
