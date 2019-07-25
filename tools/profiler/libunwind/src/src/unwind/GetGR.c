
























#include "unwind-internal.h"

PROTECTED unsigned long
_Unwind_GetGR (struct _Unwind_Context *context, int index)
{
  unw_word_t val;

  if (index == UNW_REG_SP && context->end_of_stack)
    

    return 0;

  unw_get_reg (&context->cursor, index, &val);
  return val;
}

unsigned long __libunwind_Unwind_GetGR (struct _Unwind_Context *, int)
     ALIAS (_Unwind_GetGR);
