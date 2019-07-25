























#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <inttypes.h>
#include <ucontext.h>

#ifdef mips
# undef mips
#endif

#define UNW_TARGET	mips
#define UNW_TARGET_MIPS	1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */






   

#define UNW_TDEP_CURSOR_LEN	4096





typedef uint64_t unw_word_t;
typedef int32_t unw_sword_t;


typedef long double unw_tdep_fpreg_t;

typedef enum
  {
    UNW_MIPS_R0,
    UNW_MIPS_R1,
    UNW_MIPS_R2,
    UNW_MIPS_R3,
    UNW_MIPS_R4,
    UNW_MIPS_R5,
    UNW_MIPS_R6,
    UNW_MIPS_R7,
    UNW_MIPS_R8,
    UNW_MIPS_R9,
    UNW_MIPS_R10,
    UNW_MIPS_R11,
    UNW_MIPS_R12,
    UNW_MIPS_R13,
    UNW_MIPS_R14,
    UNW_MIPS_R15,
    UNW_MIPS_R16,
    UNW_MIPS_R17,
    UNW_MIPS_R18,
    UNW_MIPS_R19,
    UNW_MIPS_R20,
    UNW_MIPS_R21,
    UNW_MIPS_R22,
    UNW_MIPS_R23,
    UNW_MIPS_R24,
    UNW_MIPS_R25,
    UNW_MIPS_R26,
    UNW_MIPS_R27,
    UNW_MIPS_R28,
    UNW_MIPS_R29,
    UNW_MIPS_R30,
    UNW_MIPS_R31,

    

    

    UNW_MIPS_CFA,

    UNW_TDEP_LAST_REG = UNW_MIPS_R31,

    UNW_TDEP_IP = UNW_MIPS_R31,
    UNW_TDEP_SP = UNW_MIPS_R29,
    UNW_TDEP_EH = UNW_MIPS_R0   
  }
mips_regnum_t;

typedef enum
  {
    UNW_MIPS_ABI_O32,
    UNW_MIPS_ABI_N32,
    UNW_MIPS_ABI_N64
  }
mips_abi_t;

#define UNW_TDEP_NUM_EH_REGS	2	/* FIXME for MIPS.  */

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




#define unw_tdep_getcontext UNW_ARCH_OBJ(getcontext)
extern int unw_tdep_getcontext (ucontext_t *uc);

#define unw_tdep_is_fpreg		UNW_ARCH_OBJ(is_fpreg)
extern int unw_tdep_is_fpreg (int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
