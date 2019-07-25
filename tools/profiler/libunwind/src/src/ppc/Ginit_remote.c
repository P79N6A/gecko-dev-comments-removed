


























#include <libunwind_i.h>

#ifdef UNW_TARGET_PPC64
#include "../ppc64/init.h"
#else
#include "../ppc32/init.h"
#endif

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

  #ifdef UNW_TARGET_PPC64
    return common_init_ppc64 (c, 0);
  #elif UNW_TARGET_PPC32
    return common_init_ppc32 (c, 0);
  #else
    #error init_remote :: NO VALID PPC ARCH!
  #endif
#endif 
}
