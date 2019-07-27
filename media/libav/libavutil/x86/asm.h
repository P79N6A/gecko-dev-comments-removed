



















#ifndef AVUTIL_X86_ASM_H
#define AVUTIL_X86_ASM_H

#include <stdint.h>
#include "config.h"

typedef struct xmm_reg { uint64_t a, b; } xmm_reg;

#if ARCH_X86_64
#    define OPSIZE "q"
#    define REG_a "rax"
#    define REG_b "rbx"
#    define REG_c "rcx"
#    define REG_d "rdx"
#    define REG_D "rdi"
#    define REG_S "rsi"
#    define PTR_SIZE "8"
typedef int64_t x86_reg;

#    define REG_SP "rsp"
#    define REG_BP "rbp"
#    define REGBP   rbp
#    define REGa    rax
#    define REGb    rbx
#    define REGc    rcx
#    define REGd    rdx
#    define REGSP   rsp

#elif ARCH_X86_32

#    define OPSIZE "l"
#    define REG_a "eax"
#    define REG_b "ebx"
#    define REG_c "ecx"
#    define REG_d "edx"
#    define REG_D "edi"
#    define REG_S "esi"
#    define PTR_SIZE "4"
typedef int32_t x86_reg;

#    define REG_SP "esp"
#    define REG_BP "ebp"
#    define REGBP   ebp
#    define REGa    eax
#    define REGb    ebx
#    define REGc    ecx
#    define REGd    edx
#    define REGSP   esp
#else
typedef int x86_reg;
#endif

#define HAVE_7REGS (ARCH_X86_64 || (HAVE_EBX_AVAILABLE && HAVE_EBP_AVAILABLE))
#define HAVE_6REGS (ARCH_X86_64 || (HAVE_EBX_AVAILABLE || HAVE_EBP_AVAILABLE))

#if ARCH_X86_64 && defined(PIC)
#    define BROKEN_RELOCATIONS 1
#endif













#if HAVE_XMM_CLOBBERS
#    define XMM_CLOBBERS(...)        __VA_ARGS__
#    define XMM_CLOBBERS_ONLY(...) : __VA_ARGS__
#else
#    define XMM_CLOBBERS(...)
#    define XMM_CLOBBERS_ONLY(...)
#endif


#define LABEL_MANGLE(a) EXTERN_PREFIX #a


#if ARCH_X86_64 && defined(PIC)
#    define LOCAL_MANGLE(a) #a "(%%rip)"
#else
#    define LOCAL_MANGLE(a) #a
#endif

#define MANGLE(a) EXTERN_PREFIX LOCAL_MANGLE(a)

#endif 
