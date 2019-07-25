
























#include <stdlib.h>

#include "unwind_i.h"
#include "offsets.h"

#ifndef UNW_REMOTE_ONLY

static inline int
local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
#if defined(__linux)
  unw_word_t dirty_partition[2048]; 
  unw_word_t val, sol, sof, pri_unat, n, pfs, bspstore, dirty_rnat;
  struct cursor *c = (struct cursor *) cursor;
  struct
    {
      unw_word_t r1;
      unw_word_t r4;
      unw_word_t r5;
      unw_word_t r6;
      unw_word_t r7;
      unw_word_t r15;
      unw_word_t r16;
      unw_word_t r17;
      unw_word_t r18;
    }
  extra;
  int ret, dirty_size;
# define GET_NAT(n)						\
  do								\
    {								\
      ret = tdep_access_reg (c, UNW_IA64_NAT + (n), &val, 0);	\
      if (ret < 0)						\
	return ret;						\
      if (val)							\
	pri_unat |= (unw_word_t) 1 << n;			\
    }								\
  while (0)

  
  if ((ret = ia64_make_proc_info (c)) < 0)
    return ret;

  

  if ((ret = ia64_get (c, c->loc[IA64_REG_R4], &extra.r4)) < 0
      || (ret = ia64_get (c, c->loc[IA64_REG_R5], &extra.r5)) < 0
      || (ret = ia64_get (c, c->loc[IA64_REG_R6], &extra.r6)) < 0
      || (ret = ia64_get (c, c->loc[IA64_REG_R7], &extra.r7)) < 0)
    return ret;

  
  pri_unat = 0;
  GET_NAT (4); GET_NAT(5);
  GET_NAT (6); GET_NAT(7);
  n = (((uintptr_t) &extra.r4) / 8 - 4) % 64;
  pri_unat = (pri_unat << n) | (pri_unat >> (64 - n));

  if (unlikely (c->sigcontext_addr))
    {
      struct sigcontext *sc = (struct sigcontext *) c->sigcontext_addr;
#     define PR_SCRATCH		0xffc0	/* p6-p15 are scratch */
#     define PR_PRESERVED	(~(PR_SCRATCH | 1))

      














      sc->sc_gr[12] = c->psp;
      c->psp = c->sigcontext_addr - c->sigcontext_off;

      sof = (c->cfm & 0x7f);
      if ((dirty_size = rbs_cover_and_flush (c, sof, dirty_partition,
					     &dirty_rnat, &bspstore)) < 0)
	return dirty_size;

      


      sc->sc_flags &= ~IA64_SC_FLAG_IN_SYSCALL;
      sc->sc_ip = c->ip;
      sc->sc_cfm = c->cfm & (((unw_word_t) 1 << 38) - 1);
      sc->sc_pr = (c->pr & ~PR_SCRATCH) | (sc->sc_pr & ~PR_PRESERVED);
      if ((ret = ia64_get (c, c->loc[IA64_REG_PFS], &sc->sc_ar_pfs)) < 0
	  || (ret = ia64_get (c, c->loc[IA64_REG_FPSR], &sc->sc_ar_fpsr)) < 0
	  || (ret = ia64_get (c, c->loc[IA64_REG_UNAT], &sc->sc_ar_unat)) < 0)
	return ret;

      sc->sc_gr[1] = c->pi.gp;
      if (c->eh_valid_mask & 0x1) sc->sc_gr[15] = c->eh_args[0];
      if (c->eh_valid_mask & 0x2) sc->sc_gr[16] = c->eh_args[1];
      if (c->eh_valid_mask & 0x4) sc->sc_gr[17] = c->eh_args[2];
      if (c->eh_valid_mask & 0x8) sc->sc_gr[18] = c->eh_args[3];
      Debug (9, "sc: r15=%lx,r16=%lx,r17=%lx,r18=%lx\n",
	     (long) sc->sc_gr[15], (long) sc->sc_gr[16],
	     (long) sc->sc_gr[17], (long) sc->sc_gr[18]);
    }
  else
    {
      

      if ((ret = ia64_get (c, c->loc[IA64_REG_PFS], &pfs)) < 0)
	return ret;
      sol = (pfs >> 7) & 0x7f;
      if ((dirty_size = rbs_cover_and_flush (c, sol, dirty_partition,
					     &dirty_rnat, &bspstore)) < 0)
	return dirty_size;

      extra.r1 = c->pi.gp;
      extra.r15 = c->eh_args[0];
      extra.r16 = c->eh_args[1];
      extra.r17 = c->eh_args[2];
      extra.r18 = c->eh_args[3];
      Debug (9, "extra: r15=%lx,r16=%lx,r17=%lx,r18=%lx\n",
	     (long) extra.r15, (long) extra.r16,
	     (long) extra.r17, (long) extra.r18);
    }
  Debug (8, "resuming at ip=%lx\n", (long) c->ip);
  ia64_install_cursor (c, pri_unat, (unw_word_t *) &extra,
		       bspstore, dirty_size, dirty_partition + dirty_size/8,
		       dirty_rnat);
#elif defined(__hpux)
  struct cursor *c = (struct cursor *) cursor;

  setcontext (c->as_arg);	
#endif
  return -UNW_EINVAL;
}

HIDDEN int
ia64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  return local_resume (as, cursor, arg);
}

#endif 

#ifndef UNW_LOCAL_ONLY

static inline int
remote_install_cursor (struct cursor *c)
{
  int (*access_reg) (unw_addr_space_t, unw_regnum_t, unw_word_t *,
		     int write, void *);
  int (*access_fpreg) (unw_addr_space_t, unw_regnum_t, unw_fpreg_t *,
		       int write, void *);
  unw_fpreg_t fpval;
  unw_word_t val;
  int reg;

#if defined(__linux) && !defined(UNW_REMOTE_ONLY)
  if (c->as == unw_local_addr_space)
    {
      




#     define MEMIFY(preg, reg)						 \
      do {								 \
	if (IA64_IS_REG_LOC (c->loc[(preg)]))				 \
	  c->loc[(preg)] = IA64_LOC_ADDR ((unw_word_t)			 \
					  tdep_uc_addr(c->as_arg, (reg), \
						       NULL), 0);	 \
      } while (0)
      MEMIFY (IA64_REG_PR,	UNW_IA64_PR);
      MEMIFY (IA64_REG_PFS,	UNW_IA64_AR_PFS);
      MEMIFY (IA64_REG_RNAT,	UNW_IA64_AR_RNAT);
      MEMIFY (IA64_REG_UNAT,	UNW_IA64_AR_UNAT);
      MEMIFY (IA64_REG_LC,	UNW_IA64_AR_LC);
      MEMIFY (IA64_REG_FPSR,	UNW_IA64_AR_FPSR);
      MEMIFY (IA64_REG_IP,	UNW_IA64_BR + 0);
      MEMIFY (IA64_REG_B1,	UNW_IA64_BR + 1);
      MEMIFY (IA64_REG_B2,	UNW_IA64_BR + 2);
      MEMIFY (IA64_REG_B3,	UNW_IA64_BR + 3);
      MEMIFY (IA64_REG_B4,	UNW_IA64_BR + 4);
      MEMIFY (IA64_REG_B5,	UNW_IA64_BR + 5);
      MEMIFY (IA64_REG_F2,	UNW_IA64_FR + 2);
      MEMIFY (IA64_REG_F3,	UNW_IA64_FR + 3);
      MEMIFY (IA64_REG_F4,	UNW_IA64_FR + 4);
      MEMIFY (IA64_REG_F5,	UNW_IA64_FR + 5);
      MEMIFY (IA64_REG_F16,	UNW_IA64_FR + 16);
      MEMIFY (IA64_REG_F17,	UNW_IA64_FR + 17);
      MEMIFY (IA64_REG_F18,	UNW_IA64_FR + 18);
      MEMIFY (IA64_REG_F19,	UNW_IA64_FR + 19);
      MEMIFY (IA64_REG_F20,	UNW_IA64_FR + 20);
      MEMIFY (IA64_REG_F21,	UNW_IA64_FR + 21);
      MEMIFY (IA64_REG_F22,	UNW_IA64_FR + 22);
      MEMIFY (IA64_REG_F23,	UNW_IA64_FR + 23);
      MEMIFY (IA64_REG_F24,	UNW_IA64_FR + 24);
      MEMIFY (IA64_REG_F25,	UNW_IA64_FR + 25);
      MEMIFY (IA64_REG_F26,	UNW_IA64_FR + 26);
      MEMIFY (IA64_REG_F27,	UNW_IA64_FR + 27);
      MEMIFY (IA64_REG_F28,	UNW_IA64_FR + 28);
      MEMIFY (IA64_REG_F29,	UNW_IA64_FR + 29);
      MEMIFY (IA64_REG_F30,	UNW_IA64_FR + 30);
      MEMIFY (IA64_REG_F31,	UNW_IA64_FR + 31);
    }
  else
#endif 
    {
      access_reg = c->as->acc.access_reg;
      access_fpreg = c->as->acc.access_fpreg;

      Debug (8, "copying out cursor state\n");

      for (reg = 0; reg <= UNW_REG_LAST; ++reg)
	{
	  if (unw_is_fpreg (reg))
	    {
	      if (tdep_access_fpreg (c, reg, &fpval, 0) >= 0)
		(*access_fpreg) (c->as, reg, &fpval, 1, c->as_arg);
	    }
	  else
	    {
	      if (tdep_access_reg (c, reg, &val, 0) >= 0)
		(*access_reg) (c->as, reg, &val, 1, c->as_arg);
	    }
	}
    }
  return (*c->as->acc.resume) (c->as, (unw_cursor_t *) c, c->as_arg);
}

#endif 

PROTECTED int
unw_resume (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;

  Debug (1, "(cursor=%p, ip=0x%016lx)\n", c, (unsigned long) c->ip);

#ifdef UNW_LOCAL_ONLY
  return local_resume (c->as, cursor, c->as_arg);
#else
  return remote_install_cursor (c);
#endif
}
