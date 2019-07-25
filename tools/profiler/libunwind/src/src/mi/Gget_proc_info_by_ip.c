
























#include "libunwind_i.h"

PROTECTED int
unw_get_proc_info_by_ip (unw_addr_space_t as, unw_word_t ip,
			 unw_proc_info_t *pi, void *as_arg)
{
  unw_accessors_t *a = unw_get_accessors (as);
  int ret;

  ret = unwi_find_dynamic_proc_info (as, ip, pi, 0, as_arg);
  if (ret == -UNW_ENOINFO)
    ret = (*a->find_proc_info) (as, ip, pi, 0, as_arg);
  return ret;
}
