
























#include "unwind-internal.h"
#ifdef UNW_TARGET_X86
#include "dwarf_i.h"
#endif

PROTECTED void
_Unwind_SetGR (struct _Unwind_Context *context, int index,
	       unsigned long new_value)
{
#ifdef UNW_TARGET_X86
  index = dwarf_to_unw_regnum(index);
#endif
  unw_set_reg (&context->cursor, index, new_value);
#ifdef UNW_TARGET_IA64
  if (index >= UNW_IA64_GR && index <= UNW_IA64_GR + 127)
    
    unw_set_reg (&context->cursor, UNW_IA64_NAT + (index - UNW_IA64_GR), 0);
#endif
}

void __libunwind_Unwind_SetGR (struct _Unwind_Context *, int, unsigned long)
     ALIAS (_Unwind_SetGR);
