


























#include <stdlib.h>

#include "offsets.h"
#include "unwind_i.h"

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
x86_64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  struct cursor *c = (struct cursor *) cursor;
  ucontext_t *uc = c->uc;

  



  dwarf_make_proc_info (&c->dwarf);

  if (unlikely (c->sigcontext_format != X86_64_SCF_NONE))
    {
      x86_64_sigreturn(cursor);
      abort();
    }
  else
    {
      Debug (8, "resuming at ip=%llx via setcontext()\n",
	     (unsigned long long) c->dwarf.ip);
      setcontext (uc);
    }
  return -UNW_EINVAL;
}

#endif 




static inline int
establish_machine_state (struct cursor *c)
{
  int (*access_reg) (unw_addr_space_t, unw_regnum_t, unw_word_t *,
		     int write, void *);
  int (*access_fpreg) (unw_addr_space_t, unw_regnum_t, unw_fpreg_t *,
		       int write, void *);
  unw_addr_space_t as = c->dwarf.as;
  void *arg = c->dwarf.as_arg;
  unw_fpreg_t fpval;
  unw_word_t val;
  int reg;

  access_reg = as->acc.access_reg;
  access_fpreg = as->acc.access_fpreg;

  Debug (8, "copying out cursor state\n");

  for (reg = 0; reg <= UNW_REG_LAST; ++reg)
    {
      Debug (16, "copying %s %d\n", unw_regname (reg), reg);
      if (unw_is_fpreg (reg))
	{
	  if (tdep_access_fpreg (c, reg, &fpval, 0) >= 0)
	    (*access_fpreg) (as, reg, &fpval, 1, arg);
	}
      else
	{
	  if (tdep_access_reg (c, reg, &val, 0) >= 0)
	    (*access_reg) (as, reg, &val, 1, arg);
	}
    }
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
