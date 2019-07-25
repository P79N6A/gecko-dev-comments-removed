
























#include "init.h"
#include "unwind_i.h"

PROTECTED int
unw_init_remote (unw_cursor_t *cursor, unw_addr_space_t as, void *as_arg)
{
#ifdef UNW_LOCAL_ONLY
  return -UNW_EINVAL;
#else 
  struct cursor *c = (struct cursor *) cursor;

  if (tdep_needs_initialization)
    tdep_init ();

  Debug (1, "(cursor=%p)\n", c);

  c->dwarf.as = as;
  c->dwarf.as_arg = as_arg;
  if (as == unw_local_addr_space)
    {
      c->dwarf.as_arg = c;
      c->uc = as_arg;
    }
  else
    {
      c->dwarf.as_arg = as_arg;
      c->uc = 0;
    }
  return common_init (c, 0);
#endif 
}
