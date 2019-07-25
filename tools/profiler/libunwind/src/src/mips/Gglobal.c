























#include "unwind_i.h"
#include "dwarf_i.h"

HIDDEN pthread_mutex_t mips_lock = PTHREAD_MUTEX_INITIALIZER;
HIDDEN int tdep_needs_initialization = 1;




HIDDEN uint8_t dwarf_to_unw_regnum_map[] =
  {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
  };

HIDDEN void
tdep_init (void)
{
  intrmask_t saved_mask;

  sigfillset (&unwi_full_mask);

  lock_acquire (&mips_lock, saved_mask);
  {
    if (!tdep_needs_initialization)
      
      goto out;

    mi_init ();

    dwarf_init ();

#ifndef UNW_REMOTE_ONLY
    mips_local_addr_space_init ();
#endif
    tdep_needs_initialization = 0;	
  }
 out:
  lock_release (&mips_lock, saved_mask);
}
