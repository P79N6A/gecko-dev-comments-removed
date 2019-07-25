

























#include <libunwind_i.h>

#ifdef UNW_TARGET_PPC64
#include "../ppc64/init.h"
#else
#include "../ppc32/init.h"
#endif

#ifdef UNW_REMOTE_ONLY

PROTECTED int
unw_init_local (unw_cursor_t *cursor, ucontext_t *uc)
{
  
  return -UNW_EINVAL;
}

#else 

PROTECTED int
unw_init_local (unw_cursor_t *cursor, ucontext_t *uc)
{
  struct cursor *c = (struct cursor *) cursor;

  if (tdep_needs_initialization)
    tdep_init ();

  Debug (1, "(cursor=%p)\n", c);

  c->dwarf.as = unw_local_addr_space;
  c->dwarf.as_arg = uc;
  #ifdef UNW_TARGET_PPC64
    return common_init_ppc64 (c, 1);
  #else
    return common_init_ppc32 (c, 1);
  #endif
}

#endif 
