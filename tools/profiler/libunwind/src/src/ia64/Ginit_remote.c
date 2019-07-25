
























#include "init.h"
#include "unwind_i.h"

PROTECTED int
unw_init_remote (unw_cursor_t *cursor, unw_addr_space_t as, void *as_arg)
{
#ifdef UNW_LOCAL_ONLY
  return -UNW_EINVAL;
#else 
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t sp, bsp;
  int ret;

  if (tdep_needs_initialization)
    tdep_init ();

  Debug (1, "(cursor=%p)\n", c);

  if (as == unw_local_addr_space)
    




    return unw_init_local (cursor, as_arg);

  c->as = as;
  c->as_arg = as_arg;

  if ((ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_GR + 12), &sp)) < 0
      || (ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_AR_BSP), &bsp)) < 0)
    return ret;

  return common_init (c, sp, bsp);
#endif 
}
