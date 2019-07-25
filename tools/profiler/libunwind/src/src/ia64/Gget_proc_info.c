
























#include "unwind_i.h"

PROTECTED int
unw_get_proc_info (unw_cursor_t *cursor, unw_proc_info_t *pi)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  if ((ret = ia64_make_proc_info (c)) < 0)
    return ret;
  *pi = c->pi;
  return 0;
}
