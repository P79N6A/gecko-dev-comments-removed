
























#include "libunwind_i.h"

PROTECTED int
unw_get_fpreg (unw_cursor_t *cursor, int regnum, unw_fpreg_t *valp)
{
  struct cursor *c = (struct cursor *) cursor;

  return tdep_access_fpreg (c, regnum, valp, 0);
}
