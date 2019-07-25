
























#include "unwind_i.h"
#include "offsets.h"

PROTECTED int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret, i;

  Debug (1, "(cursor=%p, ip=0x%08x)\n", c, (unsigned) c->dwarf.ip);

  
  ret = dwarf_step (&c->dwarf);

  if (ret < 0 && ret != -UNW_ENOINFO)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }

  if (unlikely (ret < 0))
    {
      


      Debug (13, "dwarf_step() failed (ret=%d), trying fallback\n", ret);

      if (unw_is_signal_frame (cursor))
	{
#ifdef __linux__
	  

	  unw_word_t ip, sc_addr = c->dwarf.ip + LINUX_RT_SIGFRAME_UC_OFF;
	  dwarf_loc_t iaoq_loc = DWARF_LOC (sc_addr + LINUX_SC_IAOQ_OFF, 0);

	  c->sigcontext_format = HPPA_SCF_LINUX_RT_SIGFRAME;
	  c->sigcontext_addr = sc_addr;
	  c->dwarf.ret_addr_column = UNW_HPPA_RP;

	  if ((ret = dwarf_get (&c->dwarf, iaoq_loc, &ip)) , 0)
	    {
	      Debug (2, "failed to read IAOQ[1] (ret=%d)\n", ret);
	      return ret;
	    }
	  c->dwarf.ip = ip & ~0x3;	

	  for (i = 0; i < 32; ++i)
	    {
	      c->dwarf.loc[UNW_HPPA_GR + i]
		= DWARF_LOC (sc_addr + LINUX_SC_GR_OFF + 4*i, 0);
	      c->dwarf.loc[UNW_HPPA_FR + i]
		= DWARF_LOC (sc_addr + LINUX_SC_FR_OFF + 4*i, 0);
	    }

	  if ((ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_HPPA_SP],
				&c->dwarf.cfa)) < 0)
	    {
	      Debug (2, "failed to read SP (ret=%d)\n", ret);
	      return ret;
	    }
#else
# error Implement me!
#endif
	}
      else
	c->dwarf.ip = 0;
    }
  ret = (c->dwarf.ip == 0) ? 0 : 1;
  Debug (2, "returning %d\n", ret);
  return ret;
}
