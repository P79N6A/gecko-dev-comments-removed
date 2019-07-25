
























#include "libunwind_i.h"

PROTECTED int
unw_get_reg (unw_cursor_t *cursor, int regnum, unw_word_t *valp)
{
  struct cursor *c = (struct cursor *) cursor;

  
  if (regnum == UNW_REG_IP)
    {
      *valp = tdep_get_ip (c);
      return 0;
    }

  return tdep_access_reg (c, regnum, valp, 0);
}
