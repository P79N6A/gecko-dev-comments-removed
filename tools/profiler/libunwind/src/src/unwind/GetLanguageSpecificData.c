
























#include "unwind-internal.h"

PROTECTED unsigned long
_Unwind_GetLanguageSpecificData (struct _Unwind_Context *context)
{
  unw_proc_info_t pi;

  pi.lsda = 0;
  unw_get_proc_info (&context->cursor, &pi);
  return pi.lsda;
}

unsigned long
__libunwind_Unwind_GetLanguageSpecificData (struct _Unwind_Context *)
     ALIAS (_Unwind_GetLanguageSpecificData);
