
























#include "unwind-internal.h"

PROTECTED void *
_Unwind_FindEnclosingFunction (void *ip)
{
  unw_proc_info_t pi;

  if (unw_get_proc_info_by_ip (unw_local_addr_space,
			       (unw_word_t) (uintptr_t) ip, &pi, 0)
      < 0)
    return NULL;

  return (void *) (uintptr_t) pi.start_ip;
}

void *__libunwind_Unwind_FindEnclosingFunction (void *)
     ALIAS (_Unwind_FindEnclosingFunction);
