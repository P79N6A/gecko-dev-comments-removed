
























#include "unwind-internal.h"

PROTECTED unsigned long
_Unwind_GetBSP (struct _Unwind_Context *context)
{
#ifdef UNW_TARGET_IA64
  unw_word_t val;

  unw_get_reg (&context->cursor, UNW_IA64_BSP, &val);
  return val;
#else
  return 0;
#endif
}

unsigned long __libunwind_Unwind_GetBSP (struct _Unwind_Context *)
     ALIAS (_Unwind_GetBSP);
