
























#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <sys/types.h>
#include <inttypes.h>
#include <ucontext.h>

#define UNW_TARGET	x86
#define UNW_TARGET_X86	1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */






#define UNW_TDEP_CURSOR_LEN	127

typedef uint32_t unw_word_t;
typedef int32_t unw_sword_t;

typedef union {
  struct { uint8_t b[4]; } val32;
  struct { uint8_t b[10]; } val80;
  struct { uint8_t b[16]; } val128;
} unw_tdep_fpreg_t;

typedef enum
  {
    














    UNW_X86_EAX,	
    UNW_X86_EDX,	
    UNW_X86_ECX,	
    UNW_X86_EBX,	
    UNW_X86_ESI,	
    UNW_X86_EDI,	
    UNW_X86_EBP,	
    UNW_X86_ESP,	
    UNW_X86_EIP,	
    UNW_X86_EFLAGS,	
    UNW_X86_TRAPNO,	

    
    UNW_X86_ST0,	
    UNW_X86_ST1,	
    UNW_X86_ST2,	
    UNW_X86_ST3,	
    UNW_X86_ST4,	
    UNW_X86_ST5,	
    UNW_X86_ST6,	
    UNW_X86_ST7,	

    UNW_X86_FCW,	
    UNW_X86_FSW,	
    UNW_X86_FTW,	
    UNW_X86_FOP,	
    UNW_X86_FCS,	
    UNW_X86_FIP,	
    UNW_X86_FEA,	
    UNW_X86_FDS,	

    
    UNW_X86_XMM0_lo,	
    UNW_X86_XMM0_hi,	
    UNW_X86_XMM1_lo,	
    UNW_X86_XMM1_hi,	
    UNW_X86_XMM2_lo,	
    UNW_X86_XMM2_hi,	
    UNW_X86_XMM3_lo,	
    UNW_X86_XMM3_hi,	
    UNW_X86_XMM4_lo,	
    UNW_X86_XMM4_hi,	
    UNW_X86_XMM5_lo,	
    UNW_X86_XMM5_hi,	
    UNW_X86_XMM6_lo,	
    UNW_X86_XMM6_hi,	
    UNW_X86_XMM7_lo,	
    UNW_X86_XMM7_hi,	

    UNW_X86_MXCSR,	

    
    UNW_X86_GS,		
    UNW_X86_FS,		
    UNW_X86_ES,		
    UNW_X86_DS,		
    UNW_X86_SS,		
    UNW_X86_CS,		
    UNW_X86_TSS,	
    UNW_X86_LDT,	

    
    UNW_X86_CFA,

    UNW_X86_XMM0,	
    UNW_X86_XMM1,	
    UNW_X86_XMM2,	
    UNW_X86_XMM3,	
    UNW_X86_XMM4,	
    UNW_X86_XMM5,	
    UNW_X86_XMM6,	
    UNW_X86_XMM7,	

    UNW_TDEP_LAST_REG = UNW_X86_XMM7,

    UNW_TDEP_IP = UNW_X86_EIP,
    UNW_TDEP_SP = UNW_X86_ESP,
    UNW_TDEP_EH = UNW_X86_EAX
  }
x86_regnum_t;

#define UNW_TDEP_NUM_EH_REGS	2	/* eax and edx are exception args */

typedef struct unw_tdep_save_loc
  {
    
  }
unw_tdep_save_loc_t;


typedef ucontext_t unw_tdep_context_t;

#include "libunwind-dynamic.h"

typedef struct
  {
    
  }
unw_tdep_proc_info_t;

#include "libunwind-common.h"

#define unw_tdep_getcontext		UNW_ARCH_OBJ(getcontext)
extern int unw_tdep_getcontext (unw_tdep_context_t *);

#define unw_tdep_is_fpreg		UNW_ARCH_OBJ(is_fpreg)
extern int unw_tdep_is_fpreg (int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
