























#include "unwind_i.h"
#include "dwarf_i.h"

HIDDEN pthread_mutex_t arm_lock = PTHREAD_MUTEX_INITIALIZER;
HIDDEN int tdep_needs_initialization = 1;


HIDDEN int unwi_unwind_method = UNW_ARM_METHOD_ALL;




HIDDEN uint8_t dwarf_to_unw_regnum_map[16] =
  {
    
    UNW_ARM_R0, UNW_ARM_R1, UNW_ARM_R2, UNW_ARM_R3, UNW_ARM_R4, UNW_ARM_R5,
    UNW_ARM_R6, UNW_ARM_R7, UNW_ARM_R8, UNW_ARM_R9, UNW_ARM_R10, UNW_ARM_R11,
    UNW_ARM_R12, UNW_ARM_R13, UNW_ARM_R14, UNW_ARM_R15
  };

HIDDEN void
tdep_init (void)
{
  intrmask_t saved_mask;

  sigfillset (&unwi_full_mask);

  lock_acquire (&arm_lock, saved_mask);
  {
    if (!tdep_needs_initialization)
      
      goto out;

    
    const char* str = getenv ("UNW_ARM_UNWIND_METHOD");
    if (str)
      {
        unwi_unwind_method = atoi (str);
      }

    mi_init ();

    dwarf_init ();

#ifndef UNW_REMOTE_ONLY
    arm_local_addr_space_init ();
#endif
    tdep_needs_initialization = 0;	
  }
 out:
  lock_release (&arm_lock, saved_mask);
}
