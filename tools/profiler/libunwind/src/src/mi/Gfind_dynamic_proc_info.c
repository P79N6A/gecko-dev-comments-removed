
























#include "libunwind_i.h"

#ifdef UNW_REMOTE_ONLY

static inline int
local_find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		      int need_unwind_info, void *arg)
{
  return -UNW_ENOINFO;
}

#else 

static inline int
local_find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		      int need_unwind_info, void *arg)
{
  unw_dyn_info_list_t *list;
  unw_dyn_info_t *di;

#ifndef UNW_LOCAL_ONLY
# pragma weak _U_dyn_info_list_addr
  if (!_U_dyn_info_list_addr)
    return -UNW_ENOINFO;
#endif

  list = (unw_dyn_info_list_t *) (uintptr_t) _U_dyn_info_list_addr ();
  for (di = list->first; di; di = di->next)
    if (ip >= di->start_ip && ip < di->end_ip)
      return unwi_extract_dynamic_proc_info (as, ip, pi, di, need_unwind_info,
					     arg);
  return -UNW_ENOINFO;
}

#endif 

#ifdef UNW_LOCAL_ONLY

static inline int
remote_find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		       int need_unwind_info, void *arg)
{
  return -UNW_ENOINFO;
}

#else 

static inline int
remote_find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		       int need_unwind_info, void *arg)
{
  return unwi_dyn_remote_find_proc_info (as, ip, pi, need_unwind_info, arg);
}

#endif 

HIDDEN int
unwi_find_dynamic_proc_info (unw_addr_space_t as, unw_word_t ip,
			     unw_proc_info_t *pi, int need_unwind_info,
			     void *arg)
{
  if (as == unw_local_addr_space)
    return local_find_proc_info (as, ip, pi, need_unwind_info, arg);
  else
    return remote_find_proc_info (as, ip, pi, need_unwind_info, arg);
}
