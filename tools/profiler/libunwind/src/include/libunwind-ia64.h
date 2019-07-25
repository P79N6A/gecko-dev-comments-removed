
























#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#include <inttypes.h>
#include <ucontext.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef ia64
  

# undef ia64
#endif

#ifdef __hpux
  


# undef UNW_LOCAL_ONLY
# define UNW_GENERIC_ONLY
#endif

#define UNW_TARGET	ia64
#define UNW_TARGET_IA64	1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */






#define UNW_TDEP_CURSOR_LEN	511




#define UNW_PI_FLAG_IA64_RBS_SWITCH_BIT	(UNW_PI_FLAG_FIRST_TDEP_BIT + 0)

#define UNW_PI_FLAG_IA64_RBS_SWITCH	(1 << UNW_PI_FLAG_IA64_RBS_SWITCH_BIT)

typedef uint64_t unw_word_t;
typedef int64_t unw_sword_t;





typedef union
  {
    struct { unw_word_t bits[2]; } raw;
    long double dummy;	
  }
unw_tdep_fpreg_t;

typedef struct
  {
    
  }
unw_tdep_proc_info_t;

typedef enum
  {
    



    UNW_IA64_GR = 0,			
     UNW_IA64_GP = UNW_IA64_GR + 1,
     UNW_IA64_TP = UNW_IA64_GR + 13,

    UNW_IA64_NAT = UNW_IA64_GR + 128,	

    UNW_IA64_FR = UNW_IA64_NAT + 128,	

    UNW_IA64_AR = UNW_IA64_FR + 128,	
     UNW_IA64_AR_RSC = UNW_IA64_AR + 16,
     UNW_IA64_AR_BSP = UNW_IA64_AR + 17,
     UNW_IA64_AR_BSPSTORE = UNW_IA64_AR + 18,
     UNW_IA64_AR_RNAT = UNW_IA64_AR + 19,
     UNW_IA64_AR_CSD = UNW_IA64_AR + 25,
     UNW_IA64_AR_26 = UNW_IA64_AR + 26,
     UNW_IA64_AR_SSD = UNW_IA64_AR_26,
     UNW_IA64_AR_CCV = UNW_IA64_AR + 32,
     UNW_IA64_AR_UNAT = UNW_IA64_AR + 36,
     UNW_IA64_AR_FPSR = UNW_IA64_AR + 40,
     UNW_IA64_AR_PFS = UNW_IA64_AR + 64,
     UNW_IA64_AR_LC = UNW_IA64_AR + 65,
     UNW_IA64_AR_EC = UNW_IA64_AR + 66,

    UNW_IA64_BR = UNW_IA64_AR + 128,	
      UNW_IA64_RP = UNW_IA64_BR + 0,	
    UNW_IA64_PR = UNW_IA64_BR + 8,	
    UNW_IA64_CFM,

    
    UNW_IA64_BSP,
    UNW_IA64_IP,
    UNW_IA64_SP,

    UNW_TDEP_LAST_REG = UNW_IA64_SP,

    UNW_TDEP_IP = UNW_IA64_IP,
    UNW_TDEP_SP = UNW_IA64_SP,
    UNW_TDEP_EH = UNW_IA64_GR + 15
  }
ia64_regnum_t;

#define UNW_TDEP_NUM_EH_REGS	4	/* r15-r18 are exception args */

typedef struct unw_tdep_save_loc
  {
    


    uint8_t nat_bitnr;

    
    uint8_t reserved[7];
  }
unw_tdep_save_loc_t;


typedef ucontext_t unw_tdep_context_t;

#define unw_tdep_is_fpreg(r)		((unsigned) ((r) - UNW_IA64_FR) < 128)

#include "libunwind-dynamic.h"
#include "libunwind-common.h"

#ifdef __hpux
  


# define unw_tdep_getcontext		getcontext
#else
# define unw_tdep_getcontext		UNW_ARCH_OBJ (getcontext)
  extern int unw_tdep_getcontext (unw_tdep_context_t *);
#endif







#define unw_search_ia64_unwind_table	UNW_OBJ(search_unwind_table)
extern int unw_search_ia64_unwind_table (unw_addr_space_t, unw_word_t,
					 unw_dyn_info_t *, unw_proc_info_t *,
					 int, void *);





extern unw_word_t _Uia64_find_dyn_list (unw_addr_space_t, unw_dyn_info_t *,
					void *);



extern int _Uia64_get_kernel_table (unw_dyn_info_t *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
