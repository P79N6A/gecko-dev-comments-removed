
























#include "libunwind_i.h"

HIDDEN int
unwi_extract_dynamic_proc_info (unw_addr_space_t as, unw_word_t ip,
				unw_proc_info_t *pi, unw_dyn_info_t *di,
				int need_unwind_info, void *arg)
{
  pi->start_ip = di->start_ip;
  pi->end_ip = di->end_ip;
  pi->gp = di->gp;
  pi->format = di->format;
  switch (di->format)
    {
    case UNW_INFO_FORMAT_DYNAMIC:
      pi->handler = di->u.pi.handler;
      pi->lsda = 0;
      pi->flags = di->u.pi.flags;
      pi->unwind_info_size = 0;
      if (need_unwind_info)
	pi->unwind_info = di;
      else
	pi->unwind_info = NULL;
      return 0;

    case UNW_INFO_FORMAT_TABLE:
    case UNW_INFO_FORMAT_REMOTE_TABLE:
#ifdef tdep_search_unwind_table
      
      return tdep_search_unwind_table (as, ip, di, pi, need_unwind_info, arg);
#else
      
#endif
    default:
      break;
    }
  return -UNW_EINVAL;
}
