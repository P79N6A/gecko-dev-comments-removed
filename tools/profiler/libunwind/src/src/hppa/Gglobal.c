
























#include "unwind_i.h"

HIDDEN pthread_mutex_t hppa_lock = PTHREAD_MUTEX_INITIALIZER;
HIDDEN int tdep_needs_initialization = 1;

HIDDEN void
tdep_init (void)
{
  intrmask_t saved_mask;

  sigfillset (&unwi_full_mask);

  lock_acquire (&hppa_lock, saved_mask);
  {
    if (!tdep_needs_initialization)
      
      goto out;

    mi_init ();

    dwarf_init ();

#ifndef UNW_REMOTE_ONLY
    hppa_local_addr_space_init ();
#endif
    tdep_needs_initialization = 0;	
  }
 out:
  lock_release (&hppa_lock, saved_mask);
}
