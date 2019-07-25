
























#include "unwind-internal.h"




PROTECTED unsigned long
_Unwind_GetIPInfo (struct _Unwind_Context *context, int *ip_before_insn)
{
  unw_word_t val;

  unw_get_reg (&context->cursor, UNW_REG_IP, &val);
  *ip_before_insn = unw_is_signal_frame (&context->cursor);
  return val;
}

unsigned long __libunwind_Unwind_GetIPInfo (struct _Unwind_Context *, int *)
     ALIAS (_Unwind_GetIPInfo);
