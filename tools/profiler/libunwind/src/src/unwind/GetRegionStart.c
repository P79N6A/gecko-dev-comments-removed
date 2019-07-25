
























#include "unwind-internal.h"

PROTECTED unsigned long
_Unwind_GetRegionStart (struct _Unwind_Context *context)
{
  unw_proc_info_t pi;

  pi.start_ip = 0;
  unw_get_proc_info (&context->cursor, &pi);
  return pi.start_ip;
}

unsigned long __libunwind_Unwind_GetRegionStart (struct _Unwind_Context *)
     ALIAS (_Unwind_GetRegionStart);
