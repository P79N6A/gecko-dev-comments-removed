























#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <inttypes.h>
#include <ucontext.h>

#define UNW_TARGET	hppa
#define UNW_TARGET_HPPA	1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */






#define UNW_TDEP_CURSOR_LEN	511

typedef uint32_t unw_word_t;
typedef int32_t unw_sword_t;

typedef union
  {
    struct { unw_word_t bits[2]; } raw;
    double val;
  }
unw_tdep_fpreg_t;

typedef enum
  {
    



    UNW_HPPA_GR = 0,
     UNW_HPPA_RP = 2,			
     UNW_HPPA_FP = 3,			
     UNW_HPPA_SP = UNW_HPPA_GR + 30,

    UNW_HPPA_FR = UNW_HPPA_GR + 32,

    UNW_HPPA_IP = UNW_HPPA_FR + 32,	

    

    




    UNW_HPPA_EH0 = UNW_HPPA_IP + 1,	
    UNW_HPPA_EH1 = UNW_HPPA_EH0 + 1,	
    UNW_HPPA_EH2 = UNW_HPPA_EH1 + 1,	
    UNW_HPPA_EH3 = UNW_HPPA_EH2 + 1,	

    
    UNW_HPPA_CFA,

    UNW_TDEP_LAST_REG = UNW_HPPA_IP,

    UNW_TDEP_IP = UNW_HPPA_IP,
    UNW_TDEP_SP = UNW_HPPA_SP,
    UNW_TDEP_EH = UNW_HPPA_EH0
  }
hppa_regnum_t;

#define UNW_TDEP_NUM_EH_REGS	4

typedef struct unw_tdep_save_loc
  {
    
  }
unw_tdep_save_loc_t;


typedef ucontext_t unw_tdep_context_t;

#define unw_tdep_is_fpreg(r)		((unsigned) ((r) - UNW_HPPA_FR) < 32)

#include "libunwind-dynamic.h"

typedef struct
  {
    
  }
unw_tdep_proc_info_t;

#include "libunwind-common.h"

#define unw_tdep_getcontext		UNW_ARCH_OBJ (getcontext)
extern int unw_tdep_getcontext (unw_tdep_context_t *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
