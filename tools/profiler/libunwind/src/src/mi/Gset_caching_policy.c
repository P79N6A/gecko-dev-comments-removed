
























#include "libunwind_i.h"

PROTECTED int
unw_set_caching_policy (unw_addr_space_t as, unw_caching_policy_t policy)
{
  if (tdep_needs_initialization)
    tdep_init ();

#ifndef HAVE___THREAD
  if (policy == UNW_CACHE_PER_THREAD)
    policy = UNW_CACHE_GLOBAL;
#endif

  if (policy == as->caching_policy)
    return 0;	

  as->caching_policy = policy;
  
  unw_flush_cache (as, 0, 0);
  return 0;
}
