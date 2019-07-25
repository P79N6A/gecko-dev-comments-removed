
























#include "unwind-internal.h"

PROTECTED unsigned long
_Unwind_GetCFA (struct _Unwind_Context *context)
{
  unw_word_t val;

  unw_get_reg (&context->cursor, UNW_REG_SP, &val);
  return val;
}

unsigned long __libunwind_Unwind_GetCFA (struct _Unwind_Context *)
     ALIAS (_Unwind_GetCFA);
