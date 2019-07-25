
























#include "unwind-internal.h"

PROTECTED unsigned long
_Unwind_GetTextRelBase (struct _Unwind_Context *context)
{
  return 0;
}

unsigned long __libunwind_Unwind_GetTextRelBase (struct _Unwind_Context *)
     ALIAS (_Unwind_GetTextRelBase);
