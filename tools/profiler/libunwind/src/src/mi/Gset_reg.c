
























#include "libunwind_i.h"

PROTECTED int
unw_set_reg (unw_cursor_t *cursor, int regnum, unw_word_t valp)
{
  struct cursor *c = (struct cursor *) cursor;

  return tdep_access_reg (c, regnum, &valp, 1);
}
