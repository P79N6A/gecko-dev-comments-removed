
























#include "init.h"
#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY

PROTECTED int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  return -UNW_EINVAL;
}

#else 

static inline void
set_as_arg (struct cursor *c, unw_context_t *uc)
{
#if defined(__linux) && defined(__KERNEL__)
  c->task = current;
  c->as_arg = &uc->sw;
#else
  c->as_arg = uc;
#endif
}

static inline int
get_initial_stack_pointers (struct cursor *c, unw_context_t *uc,
			    unw_word_t *sp, unw_word_t *bsp)
{
#if defined(__linux)
  unw_word_t sol, bspstore;

#ifdef __KERNEL__
  sol = (uc->sw.ar_pfs >> 7) & 0x7f;
  bspstore = uc->sw.ar_bspstore;
  *sp = uc->ksp;
# else
  sol = (uc->uc_mcontext.sc_ar_pfs >> 7) & 0x7f;
  bspstore = uc->uc_mcontext.sc_ar_bsp;
  *sp = uc->uc_mcontext.sc_gr[12];
# endif
  *bsp = rse_skip_regs (bspstore, -sol);
#elif defined(__hpux)
  int ret;

  if ((ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_GR + 12), sp)) < 0
      || (ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_AR_BSP), bsp)) < 0)
    return ret;
#else
# error Fix me.
#endif
  return 0;
}

PROTECTED int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t sp, bsp;
  int ret;

  if (tdep_needs_initialization)
    tdep_init ();

  Debug (1, "(cursor=%p)\n", c);

  c->as = unw_local_addr_space;
  set_as_arg (c, uc);

  if ((ret = get_initial_stack_pointers (c, uc, &sp, &bsp)) < 0)
    return ret;

  Debug (4, "initial bsp=%lx, sp=%lx\n", bsp, sp);

  if ((ret = common_init (c, sp, bsp)) < 0)
    return ret;

#ifdef __hpux
  

  ret = unw_step (cursor);
#endif
  return ret;
}

#endif 
