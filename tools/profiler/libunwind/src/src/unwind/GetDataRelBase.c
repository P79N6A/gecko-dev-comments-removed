
























#include "unwind-internal.h"

PROTECTED unsigned long
_Unwind_GetDataRelBase (struct _Unwind_Context *context)
{
  unw_proc_info_t pi;

  pi.gp = 0;
  unw_get_proc_info (&context->cursor, &pi);
  return pi.gp;
}

unsigned long __libunwind_Unwind_GetDataRelBase (struct _Unwind_Context *)
     ALIAS (_Unwind_GetDataRelBase);
