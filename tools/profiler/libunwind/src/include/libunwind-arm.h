























#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>

#define UNW_TARGET	arm
#define UNW_TARGET_ARM	1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */






   

#define UNW_TDEP_CURSOR_LEN	4096

typedef uint32_t unw_word_t;
typedef int32_t unw_sword_t;

typedef long double unw_tdep_fpreg_t;

typedef enum
  {
    UNW_ARM_R0,
    UNW_ARM_R1,
    UNW_ARM_R2,
    UNW_ARM_R3,
    UNW_ARM_R4,
    UNW_ARM_R5,
    UNW_ARM_R6,
    UNW_ARM_R7,
    UNW_ARM_R8,
    UNW_ARM_R9,
    UNW_ARM_R10,
    UNW_ARM_R11,
    UNW_ARM_R12,
    UNW_ARM_R13,
    UNW_ARM_R14,
    UNW_ARM_R15,
    
    
    UNW_ARM_S0 = 64,
    UNW_ARM_S1,
    UNW_ARM_S2,
    UNW_ARM_S3,
    UNW_ARM_S4,
    UNW_ARM_S5,
    UNW_ARM_S6,
    UNW_ARM_S7,
    UNW_ARM_S8,
    UNW_ARM_S9,
    UNW_ARM_S10,
    UNW_ARM_S11,
    UNW_ARM_S12,
    UNW_ARM_S13,
    UNW_ARM_S14,
    UNW_ARM_S15,
    UNW_ARM_S16,
    UNW_ARM_S17,
    UNW_ARM_S18,
    UNW_ARM_S19,
    UNW_ARM_S20,
    UNW_ARM_S21,
    UNW_ARM_S22,
    UNW_ARM_S23,
    UNW_ARM_S24,
    UNW_ARM_S25,
    UNW_ARM_S26,
    UNW_ARM_S27,
    UNW_ARM_S28,
    UNW_ARM_S29,
    UNW_ARM_S30,
    UNW_ARM_S31,
    
    
    UNW_ARM_F0 = 96,
    UNW_ARM_F1,
    UNW_ARM_F2,
    UNW_ARM_F3,
    UNW_ARM_F4,
    UNW_ARM_F5,
    UNW_ARM_F6,
    UNW_ARM_F7,
    
    
    UNW_ARM_wCGR0 = 104,
    UNW_ARM_wCGR1,
    UNW_ARM_wCGR2,
    UNW_ARM_wCGR3,
    UNW_ARM_wCGR4,
    UNW_ARM_wCGR5,
    UNW_ARM_wCGR6,
    UNW_ARM_wCGR7,
    
    
    UNW_ARM_wR0 = 112,
    UNW_ARM_wR1,
    UNW_ARM_wR2,
    UNW_ARM_wR3,
    UNW_ARM_wR4,
    UNW_ARM_wR5,
    UNW_ARM_wR6,
    UNW_ARM_wR7,
    UNW_ARM_wR8,
    UNW_ARM_wR9,
    UNW_ARM_wR10,
    UNW_ARM_wR11,
    UNW_ARM_wR12,
    UNW_ARM_wR13,
    UNW_ARM_wR14,
    UNW_ARM_wR15,
    
    
    
    
    UNW_ARM_SPSR = 128,
    UNW_ARM_SPSR_FIQ,
    UNW_ARM_SPSR_IRQ,
    UNW_ARM_SPSR_ABT,
    UNW_ARM_SPSR_UND,
    UNW_ARM_SPSR_SVC,
    
    
    UNW_ARM_R8_USR = 144,
    UNW_ARM_R9_USR,
    UNW_ARM_R10_USR,
    UNW_ARM_R11_USR,
    UNW_ARM_R12_USR,
    UNW_ARM_R13_USR,
    UNW_ARM_R14_USR,
    
    
    UNW_ARM_R8_FIQ = 151,
    UNW_ARM_R9_FIQ,
    UNW_ARM_R10_FIQ,
    UNW_ARM_R11_FIQ,
    UNW_ARM_R12_FIQ,
    UNW_ARM_R13_FIQ,
    UNW_ARM_R14_FIQ,
    
    
    UNW_ARM_R13_IRQ = 158,
    UNW_ARM_R14_IRQ,
    
    
    UNW_ARM_R13_ABT = 160,
    UNW_ARM_R14_ABT,
    
    
    UNW_ARM_R13_UND = 162,
    UNW_ARM_R14_UND,
    
    
    UNW_ARM_R13_SVC = 164,
    UNW_ARM_R14_SVC,
    
    
    UNW_ARM_wC0 = 192,
    UNW_ARM_wC1,
    UNW_ARM_wC2,
    UNW_ARM_wC3,
    UNW_ARM_wC4,
    UNW_ARM_wC5,
    UNW_ARM_wC6,
    UNW_ARM_wC7,

    
    UNW_ARM_D0 = 256,
    UNW_ARM_D1,
    UNW_ARM_D2,
    UNW_ARM_D3,
    UNW_ARM_D4,
    UNW_ARM_D5,
    UNW_ARM_D6,
    UNW_ARM_D7,
    UNW_ARM_D8,
    UNW_ARM_D9,
    UNW_ARM_D10,
    UNW_ARM_D11,
    UNW_ARM_D12,
    UNW_ARM_D13,
    UNW_ARM_D14,
    UNW_ARM_D15,
    UNW_ARM_D16,
    UNW_ARM_D17,
    UNW_ARM_D18,
    UNW_ARM_D19,
    UNW_ARM_D20,
    UNW_ARM_D21,
    UNW_ARM_D22,
    UNW_ARM_D23,
    UNW_ARM_D24,
    UNW_ARM_D25,
    UNW_ARM_D26,
    UNW_ARM_D27,
    UNW_ARM_D28,
    UNW_ARM_D29,
    UNW_ARM_D30,
    UNW_ARM_D31,

    

    UNW_ARM_CFA,

    UNW_TDEP_LAST_REG = UNW_ARM_D31,

    UNW_TDEP_IP = UNW_ARM_R14,  
    UNW_TDEP_SP = UNW_ARM_R13,
    UNW_TDEP_EH = UNW_ARM_R0   
  }
arm_regnum_t;

#define UNW_TDEP_NUM_EH_REGS	2	/* FIXME for ARM.  */

typedef struct unw_tdep_save_loc
  {
    
  }
unw_tdep_save_loc_t;




typedef struct unw_tdep_context
  {
    unsigned long regs[16];
  }
unw_tdep_context_t;




#ifndef __thumb__
#define unw_tdep_getcontext(uc) (({					\
  unw_tdep_context_t *unw_ctx = (uc);					\
  register unsigned long *unw_base asm ("r0") = unw_ctx->regs;		\
  __asm__ __volatile__ (						\
    "stmia %[base], {r0-r15}"						\
    : : [base] "r" (unw_base) : "memory");				\
  }), 0)
#else 
#define unw_tdep_getcontext(uc) (({					\
  unw_tdep_context_t *unw_ctx = (uc);					\
  register unsigned long *unw_base asm ("r0") = unw_ctx->regs;		\
  __asm__ __volatile__ (						\
    ".align 2\nbx pc\nnop\n.code 32\n"					\
    "stmia %[base], {r0-r15}\n"						\
    "orr %[base], pc, #1\nbx %[base]"					\
    : [base] "+r" (unw_base) : : "memory", "cc");			\
  }), 0)
#endif

#include "libunwind-dynamic.h"

typedef struct
  {
    
  }
unw_tdep_proc_info_t;

#include "libunwind-common.h"

#define unw_tdep_is_fpreg		UNW_ARCH_OBJ(is_fpreg)
extern int unw_tdep_is_fpreg (int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
