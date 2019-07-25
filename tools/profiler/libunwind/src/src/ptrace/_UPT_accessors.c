
























#include "_UPT_internal.h"

PROTECTED unw_accessors_t _UPT_accessors =
  {
    .find_proc_info		= _UPT_find_proc_info,
    .put_unwind_info		= _UPT_put_unwind_info,
    .get_dyn_info_list_addr	= _UPT_get_dyn_info_list_addr,
    .access_mem			= _UPT_access_mem,
    .access_reg			= _UPT_access_reg,
    .access_fpreg		= _UPT_access_fpreg,
    .resume			= _UPT_resume,
    .get_proc_name		= _UPT_get_proc_name
  };
