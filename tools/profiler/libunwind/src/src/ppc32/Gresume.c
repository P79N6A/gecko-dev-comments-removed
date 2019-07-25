


























#include <stdlib.h>

#include "unwind_i.h"

#ifndef UNW_REMOTE_ONLY

#include <sys/syscall.h>



static NORETURN inline long
my_rt_sigreturn (void *new_sp)
{
  
  abort ();
}

HIDDEN inline int
ppc32_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  
  return -UNW_EINVAL;
}

#endif 




static inline int
establish_machine_state (struct cursor *c)
{
  
  return 0;
}

PROTECTED int
unw_resume (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  Debug (1, "(cursor=%p)\n", c);

  if ((ret = establish_machine_state (c)) < 0)
    return ret;

  return (*c->dwarf.as->acc.resume) (c->dwarf.as, (unw_cursor_t *) c,
				     c->dwarf.as_arg);
}
