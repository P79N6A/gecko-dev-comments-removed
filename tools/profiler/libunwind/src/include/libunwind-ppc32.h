






























#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <inttypes.h>
#include <ucontext.h>

#define UNW_TARGET		ppc32
#define UNW_TARGET_PPC32	1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */



















#define UNW_TDEP_CURSOR_LEN 200

#if __WORDSIZE==32
typedef uint32_t unw_word_t;
typedef int32_t unw_sword_t;
#else
typedef uint64_t unw_word_t;
typedef int64_t unw_sword_t;
#endif

typedef long double unw_tdep_fpreg_t;

typedef enum
  {
    UNW_PPC32_R0,
    UNW_PPC32_R1, 
    UNW_PPC32_R2,
    UNW_PPC32_R3,
    UNW_PPC32_R4,
    UNW_PPC32_R5,
    UNW_PPC32_R6,
    UNW_PPC32_R7,
    UNW_PPC32_R8,
    UNW_PPC32_R9,
    UNW_PPC32_R10,
    UNW_PPC32_R11, 
    UNW_PPC32_R12,
    UNW_PPC32_R13,
    UNW_PPC32_R14,
    UNW_PPC32_R15,
    UNW_PPC32_R16,
    UNW_PPC32_R17,
    UNW_PPC32_R18,
    UNW_PPC32_R19,
    UNW_PPC32_R20,
    UNW_PPC32_R21,
    UNW_PPC32_R22,
    UNW_PPC32_R23,
    UNW_PPC32_R24,
    UNW_PPC32_R25,
    UNW_PPC32_R26,
    UNW_PPC32_R27,
    UNW_PPC32_R28,
    UNW_PPC32_R29,
    UNW_PPC32_R30,
    UNW_PPC32_R31, 

    
    UNW_PPC32_CTR = 32,
    
    UNW_PPC32_XER = 33,
    
    UNW_PPC32_CCR = 34,
    
    
    
    
    
    UNW_PPC32_LR = 36,
    
    UNW_PPC32_FPSCR = 37,

    UNW_PPC32_F0 = 48,
    UNW_PPC32_F1,
    UNW_PPC32_F2,
    UNW_PPC32_F3,
    UNW_PPC32_F4,
    UNW_PPC32_F5,
    UNW_PPC32_F6,
    UNW_PPC32_F7,
    UNW_PPC32_F8,
    UNW_PPC32_F9,
    UNW_PPC32_F10,
    UNW_PPC32_F11,
    UNW_PPC32_F12,
    UNW_PPC32_F13,
    UNW_PPC32_F14,
    UNW_PPC32_F15,
    UNW_PPC32_F16,
    UNW_PPC32_F17,
    UNW_PPC32_F18,
    UNW_PPC32_F19,
    UNW_PPC32_F20,
    UNW_PPC32_F21,
    UNW_PPC32_F22,
    UNW_PPC32_F23,
    UNW_PPC32_F24,
    UNW_PPC32_F25,
    UNW_PPC32_F26,
    UNW_PPC32_F27,
    UNW_PPC32_F28,
    UNW_PPC32_F29,
    UNW_PPC32_F30,
    UNW_PPC32_F31,

    UNW_TDEP_LAST_REG = UNW_PPC32_F31,

    UNW_TDEP_IP = UNW_PPC32_LR,
    UNW_TDEP_SP = UNW_PPC32_R1,
    UNW_TDEP_EH = UNW_PPC32_R12
  }
ppc32_regnum_t;






#define UNW_TDEP_NUM_EH_REGS	4

typedef struct unw_tdep_save_loc
  {
    
  }
unw_tdep_save_loc_t;


typedef ucontext_t unw_tdep_context_t;





#define unw_tdep_getcontext(uc)		(getcontext (uc), 0)

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
