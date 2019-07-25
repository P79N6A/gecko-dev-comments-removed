


























#include "unwind_i.h"

int
tdep_get_func_addr (unw_addr_space_t as, unw_word_t addr,
		    unw_word_t *entry_point)
{
  unw_accessors_t *a;
  int ret;

  a = unw_get_accessors (as);
  


  ret = (*a->access_mem) (as, addr, entry_point, 0, NULL);
  if (ret < 0)
    return ret;
  return 0;
}
