
























#if UNW_TARGET_IA64

#include "libunwind_i.h"
#include "tdep-ia64/rse.h"

static inline int
bsp_match (unw_cursor_t *c, unw_word_t *wp)
{
  unw_word_t bsp, pfs, sol;

  if (unw_get_reg (c, UNW_IA64_BSP, &bsp) < 0
      || unw_get_reg (c, UNW_IA64_AR_PFS, &pfs) < 0)
    abort ();

  
  sol = (pfs >> 7) & 0x7f;
  bsp = rse_skip_regs (bsp, sol);

  if (bsp != wp[JB_BSP])
    return 0;

  if (unlikely (sol == 0))
    {
      unw_word_t sp, prev_sp;
      unw_cursor_t tmp = *c;

      




      if (unw_step (&tmp) < 0)
	abort ();

      if (unw_get_reg (&tmp, UNW_REG_SP, &sp) < 0
	  || unw_get_reg (&tmp, UNW_REG_SP, &prev_sp) < 0)
	abort ();

      if (sp == prev_sp)
	
	return 0;
    }
  return 1;
}








static inline int
resume_restores_sigmask (unw_cursor_t *c, unw_word_t *wp)
{
  unw_word_t sc_addr = ((struct cursor *) c)->sigcontext_addr;
  struct sigcontext *sc = (struct sigcontext *) sc_addr;
  sigset_t current_mask;
  void *mp;

  if (!sc_addr)
    return 0;

  

  if (wp[JB_MASK_SAVED])
    mp = &wp[JB_MASK];
  else
    {
      if (sigprocmask (SIG_BLOCK, NULL, &current_mask) < 0)
	abort ();
      mp = &current_mask;
    }
  memcpy (&sc->sc_mask, mp, sizeof (sc->sc_mask));
  return 1;
}

#else 

static inline int
bsp_match (unw_cursor_t *c, unw_word_t *wp)
{
  return 1;
}

static inline int
resume_restores_sigmask (unw_cursor_t *c, unw_word_t *wp)
{
  
  return 0;
}

#endif 
