
























#include "offsets.h"
#include "unwind_i.h"

static inline int
linux_sigtramp (struct cursor *c, ia64_loc_t prev_cfm_loc,
		unw_word_t *num_regsp)
{
#if defined(UNW_LOCAL_ONLY) && !defined(__linux)
  return -UNW_EINVAL;
#else
  unw_word_t sc_addr;
  int ret;

  if ((ret = ia64_get (c, IA64_LOC_ADDR (c->sp + 0x10
					 + LINUX_SIGFRAME_ARG2_OFF, 0),
		       &sc_addr)) < 0)
    return ret;

  c->sigcontext_addr = sc_addr;

  if (!IA64_IS_REG_LOC (c->loc[IA64_REG_IP])
      && IA64_GET_ADDR (c->loc[IA64_REG_IP]) == sc_addr + LINUX_SC_BR_OFF + 8)
    {
      

      c->loc[IA64_REG_IP]  = IA64_LOC_ADDR (sc_addr + LINUX_SC_IP_OFF, 0);
      c->cfm_loc = IA64_LOC_ADDR (sc_addr + LINUX_SC_CFM_OFF, 0);
    }

  
  c->loc[IA64_REG_PFS] = IA64_LOC_ADDR (sc_addr + LINUX_SC_AR_PFS_OFF, 0);
  c->ec_loc = prev_cfm_loc;
  *num_regsp = c->cfm & 0x7f;		
  return 0;
#endif
}

static inline int
linux_interrupt (struct cursor *c, ia64_loc_t prev_cfm_loc,
		 unw_word_t *num_regsp, int marker)
{
#if defined(UNW_LOCAL_ONLY) && !(defined(__linux) && defined(__KERNEL__))
  return -UNW_EINVAL;
#else
  unw_word_t sc_addr, num_regs;
  ia64_loc_t pfs_loc;

  sc_addr = c->sigcontext_addr = c->sp + 0x10;

  if ((c->pr & (1UL << LINUX_PT_P_NONSYS)) != 0)
    num_regs = c->cfm & 0x7f;
  else
    num_regs = 0;

  
  if (marker == ABI_MARKER_OLD_LINUX_INTERRUPT)
	  pfs_loc = IA64_LOC_ADDR (sc_addr + LINUX_OLD_PT_PFS_OFF, 0);
  else
	  pfs_loc = IA64_LOC_ADDR (sc_addr + LINUX_PT_PFS_OFF, 0);
  c->loc[IA64_REG_PFS] = pfs_loc;
  c->ec_loc = prev_cfm_loc;
  *num_regsp = num_regs;		
  return 0;
#endif
}

static inline int
hpux_sigtramp (struct cursor *c, ia64_loc_t prev_cfm_loc,
	       unw_word_t *num_regsp)
{
#if defined(UNW_LOCAL_ONLY) && !defined(__hpux)
  return -UNW_EINVAL;
#else
  unw_word_t sc_addr, bsp, bspstore;
  ia64_loc_t sc_loc;
  int ret, i;

  
  if ((ret = ia64_get_stacked (c, 32, &sc_loc, NULL)) < 0)
    return ret;
  if ((ret = ia64_get (c, sc_loc, &sc_addr)) < 0)
    return ret;

  c->sigcontext_addr = sc_addr;

  

  c->cfm_loc = IA64_LOC_UC_REG (UNW_IA64_CFM, sc_addr);
  c->loc[IA64_REG_PRI_UNAT_MEM] = IA64_NULL_LOC;
  c->loc[IA64_REG_PSP] = IA64_LOC_UC_REG (UNW_IA64_GR + 12, sc_addr);
  c->loc[IA64_REG_BSP] = IA64_LOC_UC_REG (UNW_IA64_AR_BSP, sc_addr);
  c->loc[IA64_REG_BSPSTORE] = IA64_LOC_UC_REG (UNW_IA64_AR_BSPSTORE, sc_addr);
  c->loc[IA64_REG_PFS] = IA64_LOC_UC_REG (UNW_IA64_AR_PFS, sc_addr);
  c->loc[IA64_REG_RNAT] = IA64_LOC_UC_REG (UNW_IA64_AR_RNAT, sc_addr);
  c->loc[IA64_REG_IP] = IA64_LOC_UC_REG (UNW_IA64_IP, sc_addr);
  c->loc[IA64_REG_R4] = IA64_LOC_UC_REG (UNW_IA64_GR + 4, sc_addr);
  c->loc[IA64_REG_R5] = IA64_LOC_UC_REG (UNW_IA64_GR + 5, sc_addr);
  c->loc[IA64_REG_R6] = IA64_LOC_UC_REG (UNW_IA64_GR + 6, sc_addr);
  c->loc[IA64_REG_R7] = IA64_LOC_UC_REG (UNW_IA64_GR + 7, sc_addr);
  c->loc[IA64_REG_NAT4] = IA64_LOC_UC_REG (UNW_IA64_NAT + 4, sc_addr);
  c->loc[IA64_REG_NAT5] = IA64_LOC_UC_REG (UNW_IA64_NAT + 5, sc_addr);
  c->loc[IA64_REG_NAT6] = IA64_LOC_UC_REG (UNW_IA64_NAT + 6, sc_addr);
  c->loc[IA64_REG_NAT7] = IA64_LOC_UC_REG (UNW_IA64_NAT + 7, sc_addr);
  c->loc[IA64_REG_UNAT] = IA64_LOC_UC_REG (UNW_IA64_AR_UNAT, sc_addr);
  c->loc[IA64_REG_PR] = IA64_LOC_UC_REG (UNW_IA64_PR, sc_addr);
  c->loc[IA64_REG_LC] = IA64_LOC_UC_REG (UNW_IA64_AR_LC, sc_addr);
  c->loc[IA64_REG_FPSR] = IA64_LOC_UC_REG (UNW_IA64_AR_FPSR, sc_addr);
  c->loc[IA64_REG_B1] = IA64_LOC_UC_REG (UNW_IA64_BR + 1, sc_addr);
  c->loc[IA64_REG_B2] = IA64_LOC_UC_REG (UNW_IA64_BR + 2, sc_addr);
  c->loc[IA64_REG_B3] = IA64_LOC_UC_REG (UNW_IA64_BR + 3, sc_addr);
  c->loc[IA64_REG_B4] = IA64_LOC_UC_REG (UNW_IA64_BR + 4, sc_addr);
  c->loc[IA64_REG_B5] = IA64_LOC_UC_REG (UNW_IA64_BR + 5, sc_addr);
  c->loc[IA64_REG_F2] = IA64_LOC_UC_REG (UNW_IA64_FR + 2, sc_addr);
  c->loc[IA64_REG_F3] = IA64_LOC_UC_REG (UNW_IA64_FR + 3, sc_addr);
  c->loc[IA64_REG_F4] = IA64_LOC_UC_REG (UNW_IA64_FR + 4, sc_addr);
  c->loc[IA64_REG_F5] = IA64_LOC_UC_REG (UNW_IA64_FR + 5, sc_addr);
  for (i = 0; i < 16; ++i)
    c->loc[IA64_REG_F16 + i] = IA64_LOC_UC_REG (UNW_IA64_FR + 16 + i, sc_addr);

  c->pi.flags |= UNW_PI_FLAG_IA64_RBS_SWITCH;

  
  if ((ret = ia64_get (c, c->cfm_loc, &c->cfm)) < 0)
    return ret;
  
  if ((ret = ia64_get (c, c->loc[IA64_REG_PSP], &c->psp)) < 0)
    return ret;

  if ((ret = ia64_get (c, c->loc[IA64_REG_BSP], &bsp)) < 0
      || (ret = ia64_get (c, c->loc[IA64_REG_BSPSTORE], &bspstore)) < 0)
    return ret;
  if (bspstore < bsp)
    

    rbs_switch (c, bsp, bspstore, IA64_LOC_UC_ADDR (bsp | 0x1f8, 0));

  c->ec_loc = prev_cfm_loc;

  *num_regsp = 0;
  return 0;
#endif
}


static inline int
check_rbs_switch (struct cursor *c)
{
  unw_word_t saved_bsp, saved_bspstore, loadrs, ndirty;
  int ret = 0;

  saved_bsp = c->bsp;
  if (c->pi.flags & UNW_PI_FLAG_IA64_RBS_SWITCH)
    {
      

      if ((ret = ia64_get (c, c->loc[IA64_REG_BSP], &saved_bsp)) < 0
	  || (ret = ia64_get (c, c->loc[IA64_REG_BSPSTORE], &saved_bspstore)))
	return ret;
    }
  else if ((c->abi_marker == ABI_MARKER_LINUX_SIGTRAMP
	    || c->abi_marker == ABI_MARKER_OLD_LINUX_SIGTRAMP)
	   && !IA64_IS_REG_LOC (c->loc[IA64_REG_BSP])
	   && (IA64_GET_ADDR (c->loc[IA64_REG_BSP])
	       == c->sigcontext_addr + LINUX_SC_AR_BSP_OFF))
    {
      







      



      if ((ret = ia64_get (c, IA64_LOC_ADDR (c->sigcontext_addr
					     + LINUX_SC_AR_BSP_OFF, 0),
			   &saved_bsp) < 0)
	  || (ret = ia64_get (c, IA64_LOC_ADDR (c->sigcontext_addr
						+ LINUX_SC_LOADRS_OFF, 0),
			      &loadrs) < 0))
	return ret;
      loadrs >>= 16;
      ndirty = rse_num_regs (c->bsp - loadrs, c->bsp);
      saved_bspstore = rse_skip_regs (saved_bsp, -ndirty);
    }

  if (saved_bsp == c->bsp)
    return 0;

  return rbs_switch (c, saved_bsp, saved_bspstore, c->loc[IA64_REG_RNAT]);
}

static inline int
update_frame_state (struct cursor *c)
{
  unw_word_t prev_ip, prev_sp, prev_bsp, ip, num_regs;
  ia64_loc_t prev_cfm_loc;
  int ret;

  prev_cfm_loc = c->cfm_loc;
  prev_ip = c->ip;
  prev_sp = c->sp;
  prev_bsp = c->bsp;

  


  ret = ia64_get (c, c->loc[IA64_REG_IP], &ip);
  if (ret < 0)
    return ret;
  c->ip = ip;

  if ((ip & 0xc) != 0)
    {
      
      Debug (1, "rejecting bad ip=0x%lx\n", (long) c->ip);
      return -UNW_EINVALIDIP;
    }

  c->cfm_loc = c->loc[IA64_REG_PFS];
  
  ret = ia64_get (c, c->cfm_loc, &c->cfm);
  if (ret < 0)
    return ret;

  








  c->ec_loc = c->cfm_loc;

  num_regs = 0;
  if (unlikely (c->abi_marker))
    {
      c->last_abi_marker = c->abi_marker;
      switch (ia64_get_abi_marker (c))
	{
	case ABI_MARKER_LINUX_SIGTRAMP:
	case ABI_MARKER_OLD_LINUX_SIGTRAMP:
	  ia64_set_abi (c, ABI_LINUX);
	  if ((ret = linux_sigtramp (c, prev_cfm_loc, &num_regs)) < 0)
	    return ret;
	  break;

	case ABI_MARKER_OLD_LINUX_INTERRUPT:
	case ABI_MARKER_LINUX_INTERRUPT:
	  ia64_set_abi (c, ABI_LINUX);
	  if ((ret = linux_interrupt (c, prev_cfm_loc, &num_regs,
				      c->abi_marker)) < 0)
	    return ret;
	  break;

	case ABI_MARKER_HP_UX_SIGTRAMP:
	  ia64_set_abi (c, ABI_HPUX);
	  if ((ret = hpux_sigtramp (c, prev_cfm_loc, &num_regs)) < 0)
	    return ret;
	  break;

	default:
	  Debug (1, "unknown ABI marker: ABI=%u, context=%u\n",
		 c->abi_marker >> 8, c->abi_marker & 0xff);
	  return -UNW_EINVAL;
	}
      Debug (12, "sigcontext_addr=%lx (ret=%d)\n",
	     (unsigned long) c->sigcontext_addr, ret);

      c->sigcontext_off = c->sigcontext_addr - c->sp;

      
      if ((ret = ia64_get (c, c->loc[IA64_REG_IP], &ip)) < 0)
 	return ret;
      c->ip = ip;
      if (ip == 0)
	
	return 0;
    }
  else
    num_regs = (c->cfm >> 7) & 0x7f;	

  if (!IA64_IS_NULL_LOC (c->loc[IA64_REG_BSP]))
    {
      ret = check_rbs_switch (c);
      if (ret < 0)
	return ret;
    }

  c->bsp = rse_skip_regs (c->bsp, -num_regs);

  c->sp = c->psp;
  c->abi_marker = 0;

  if (c->ip == prev_ip && c->sp == prev_sp && c->bsp == prev_bsp)
    {
      Dprintf ("%s: ip, sp, and bsp unchanged; stopping here (ip=0x%lx)\n",
	       __FUNCTION__, (long) ip);
      return -UNW_EBADFRAME;
    }

  
  c->loc[IA64_REG_PRI_UNAT_MEM] = c->loc[IA64_REG_UNAT];

  
  ret = ia64_get (c, c->loc[IA64_REG_PR], &c->pr);
  if (ret < 0)
    return ret;

  c->pi_valid = 0;
  return 0;
}


PROTECTED int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  Debug (1, "(cursor=%p, ip=0x%016lx)\n", c, (unsigned long) c->ip);

  if ((ret = ia64_find_save_locs (c)) >= 0
      && (ret = update_frame_state (c)) >= 0)
    ret = (c->ip == 0) ? 0 : 1;

  Debug (2, "returning %d (ip=0x%016lx)\n", ret, (unsigned long) c->ip);
  return ret;
}
