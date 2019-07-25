






























#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <inttypes.h>
#include <ucontext.h>

#define UNW_TARGET		ppc64
#define UNW_TARGET_PPC64	1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */



















#define UNW_TDEP_CURSOR_LEN 280

#if __WORDSIZE==32
typedef uint32_t unw_word_t;
typedef int32_t unw_sword_t;
#else
typedef uint64_t unw_word_t;
typedef int64_t unw_sword_t;
#endif

typedef long double unw_tdep_fpreg_t;




typedef struct {
    uint64_t halves[2];
} unw_tdep_vreg_t;

typedef enum
  {
    UNW_PPC64_R0,
    UNW_PPC64_R1, 
    UNW_PPC64_R2,
    UNW_PPC64_R3,
    UNW_PPC64_R4,
    UNW_PPC64_R5,
    UNW_PPC64_R6,
    UNW_PPC64_R7,
    UNW_PPC64_R8,
    UNW_PPC64_R9,
    UNW_PPC64_R10,
    UNW_PPC64_R11, 
    UNW_PPC64_R12,
    UNW_PPC64_R13,
    UNW_PPC64_R14,
    UNW_PPC64_R15,
    UNW_PPC64_R16,
    UNW_PPC64_R17,
    UNW_PPC64_R18,
    UNW_PPC64_R19,
    UNW_PPC64_R20,
    UNW_PPC64_R21,
    UNW_PPC64_R22,
    UNW_PPC64_R23,
    UNW_PPC64_R24,
    UNW_PPC64_R25,
    UNW_PPC64_R26,
    UNW_PPC64_R27,
    UNW_PPC64_R28,
    UNW_PPC64_R29,
    UNW_PPC64_R30,
    UNW_PPC64_R31, 

    UNW_PPC64_F0 = 32,
    UNW_PPC64_F1,
    UNW_PPC64_F2,
    UNW_PPC64_F3,
    UNW_PPC64_F4,
    UNW_PPC64_F5,
    UNW_PPC64_F6,
    UNW_PPC64_F7,
    UNW_PPC64_F8,
    UNW_PPC64_F9,
    UNW_PPC64_F10,
    UNW_PPC64_F11,
    UNW_PPC64_F12,
    UNW_PPC64_F13,
    UNW_PPC64_F14,
    UNW_PPC64_F15,
    UNW_PPC64_F16,
    UNW_PPC64_F17,
    UNW_PPC64_F18,
    UNW_PPC64_F19,
    UNW_PPC64_F20,
    UNW_PPC64_F21,
    UNW_PPC64_F22,
    UNW_PPC64_F23,
    UNW_PPC64_F24,
    UNW_PPC64_F25,
    UNW_PPC64_F26,
    UNW_PPC64_F27,
    UNW_PPC64_F28,
    UNW_PPC64_F29,
    UNW_PPC64_F30,
    UNW_PPC64_F31,
    






    UNW_PPC64_LR = 65,
    UNW_PPC64_CTR = 66,
    UNW_PPC64_ARG_POINTER = 67,

    UNW_PPC64_CR0 = 68,
    UNW_PPC64_CR1,
    UNW_PPC64_CR2,
    UNW_PPC64_CR3,
    UNW_PPC64_CR4,
    
    UNW_PPC64_CR5,
    UNW_PPC64_CR6,
    UNW_PPC64_CR7,

    UNW_PPC64_XER = 76,

    UNW_PPC64_V0 = 77,
    UNW_PPC64_V1,
    UNW_PPC64_V2,
    UNW_PPC64_V3,
    UNW_PPC64_V4,
    UNW_PPC64_V5,
    UNW_PPC64_V6,
    UNW_PPC64_V7,
    UNW_PPC64_V8,
    UNW_PPC64_V9,
    UNW_PPC64_V10,
    UNW_PPC64_V11,
    UNW_PPC64_V12,
    UNW_PPC64_V13,
    UNW_PPC64_V14,
    UNW_PPC64_V15,
    UNW_PPC64_V16,
    UNW_PPC64_V17,
    UNW_PPC64_V18,
    UNW_PPC64_V19,
    UNW_PPC64_V20,
    UNW_PPC64_V21,
    UNW_PPC64_V22,
    UNW_PPC64_V23,
    UNW_PPC64_V24,
    UNW_PPC64_V25,
    UNW_PPC64_V26,
    UNW_PPC64_V27,
    UNW_PPC64_V28,
    UNW_PPC64_V29,
    UNW_PPC64_V30,
    UNW_PPC64_V31,

    UNW_PPC64_VRSAVE = 109,
    UNW_PPC64_VSCR = 110,
    UNW_PPC64_SPE_ACC = 111,
    UNW_PPC64_SPEFSCR = 112,

    
    UNW_PPC64_FRAME_POINTER,
    UNW_PPC64_NIP,


    UNW_TDEP_LAST_REG = UNW_PPC64_NIP,

    UNW_TDEP_IP = UNW_PPC64_NIP,
    UNW_TDEP_SP = UNW_PPC64_R1,
    UNW_TDEP_EH = UNW_PPC64_R12
  }
ppc64_regnum_t;






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
