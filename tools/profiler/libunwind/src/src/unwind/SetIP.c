
























#include "unwind-internal.h"

PROTECTED void
_Unwind_SetIP (struct _Unwind_Context *context, unsigned long new_value)
{
  unw_set_reg (&context->cursor, UNW_REG_IP, new_value);
}

void __libunwind_Unwind_SetIP (struct _Unwind_Context *, unsigned long)
     ALIAS (_Unwind_SetIP);
