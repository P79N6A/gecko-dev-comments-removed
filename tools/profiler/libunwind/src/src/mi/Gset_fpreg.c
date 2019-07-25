
























#include "libunwind_i.h"

PROTECTED int
unw_set_fpreg (unw_cursor_t *cursor, int regnum, unw_fpreg_t val)
{
  struct cursor *c = (struct cursor *) cursor;

  return tdep_access_fpreg (c, regnum, &val, 1);
}
