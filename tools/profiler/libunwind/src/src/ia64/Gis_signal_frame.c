
























#include "unwind_i.h"

PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  struct ia64_state_record sr;
  int ret;

  


  ret = ia64_fetch_proc_info (c, c->ip, 1);
  if (ret < 0)
    return ret;

  ret = ia64_create_state_record (c, &sr);
  if (ret < 0)
    return ret;

  

  ret = (sr.abi_marker != 0);

  ia64_free_state_record (&sr);

  Debug (1, "(cursor=%p, ip=0x%016lx) -> %d\n", c, c->ip, ret);
  return ret;
}
