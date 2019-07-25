
























#include "libunwind_i.h"

HIDDEN void
unwi_put_dynamic_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi,
			      void *arg)
{
  switch (pi->format)
    {
    case UNW_INFO_FORMAT_DYNAMIC:
#ifndef UNW_LOCAL_ONLY
# ifdef UNW_REMOTE_ONLY
      unwi_dyn_remote_put_unwind_info (as, pi, arg);
# else
      if (as != unw_local_addr_space)
	unwi_dyn_remote_put_unwind_info (as, pi, arg);
# endif
#endif
      break;

    case UNW_INFO_FORMAT_TABLE:
    case UNW_INFO_FORMAT_REMOTE_TABLE:
#ifdef tdep_put_unwind_info
      tdep_put_unwind_info (as, pi, arg);
      break;
#endif

    default:
      break;
    }
}
