























#include "unwind_i.h"

PROTECTED int
unw_get_proc_info (unw_cursor_t *cursor, unw_proc_info_t *pi)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  

  ret = dwarf_make_proc_info (&c->dwarf);
  if (ret < 0)
    return ret;

  *pi = c->dwarf.pi;
  return 0;
}
