


























#include "unwind_i.h"
#include "ucontext_i.h"
#include <signal.h>







#define __SIGNAL_FRAMESIZE 128





typedef struct
{
  long unsigned back_chain;
  long unsigned lr_save;
  
} stack_frame_t;


PROTECTED int
unw_step (unw_cursor_t * cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  stack_frame_t dummy;
  unw_word_t back_chain_offset, lr_save_offset;
  struct dwarf_loc back_chain_loc, lr_save_loc, sp_loc, ip_loc;
  int ret;

  Debug (1, "(cursor=%p, ip=0x%016lx)\n", c, (unsigned long) c->dwarf.ip);

  if (c->dwarf.ip == 0)
    {
      

      return 0;
    }

  

  ret = dwarf_step (&c->dwarf);

  if (ret < 0 && ret != -UNW_ENOINFO)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }

  if (unlikely (ret < 0))
    {
      if (likely (!unw_is_signal_frame (cursor)))
	{
	  









	  back_chain_offset = ((void *) &dummy.back_chain - (void *) &dummy);
	  lr_save_offset = ((void *) &dummy.lr_save - (void *) &dummy);

	  back_chain_loc = DWARF_LOC (c->dwarf.cfa + back_chain_offset, 0);

	  if ((ret =
	       dwarf_get (&c->dwarf, back_chain_loc, &c->dwarf.cfa)) < 0)
	    {
	      Debug
		("Unable to retrieve CFA from back chain in stack frame - %d\n",
		 ret);
	      return ret;
	    }
	  if (c->dwarf.cfa == 0)
	    

	    return 0;

	  lr_save_loc = DWARF_LOC (c->dwarf.cfa + lr_save_offset, 0);

	  if ((ret = dwarf_get (&c->dwarf, lr_save_loc, &c->dwarf.ip)) < 0)
	    {
	      Debug
		("Unable to retrieve IP from lr save in stack frame - %d\n",
		 ret);
	      return ret;
	    }
	  ret = 1;
	}
      else
	{
          









	  unw_word_t ucontext = c->dwarf.cfa + __SIGNAL_FRAMESIZE;

	  Debug (1, "signal frame, skip over trampoline\n");

	  c->sigcontext_format = PPC_SCF_LINUX_RT_SIGFRAME;
	  c->sigcontext_addr = ucontext;

	  sp_loc = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R1, 0);
	  ip_loc = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_LINK, 0);

	  ret = dwarf_get (&c->dwarf, sp_loc, &c->dwarf.cfa);
	  if (ret < 0)
	    {
	      Debug (2, "returning %d\n", ret);
	      return ret;
	    }
	  ret = dwarf_get (&c->dwarf, ip_loc, &c->dwarf.ip);
	  if (ret < 0)
	    {
	      Debug (2, "returning %d\n", ret);
	      return ret;
	    }

	  



	  c->dwarf.loc[UNW_PPC32_R0] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R0, 0);
	  c->dwarf.loc[UNW_PPC32_R1] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R1, 0);
	  c->dwarf.loc[UNW_PPC32_R2] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R2, 0);
	  c->dwarf.loc[UNW_PPC32_R3] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R3, 0);
	  c->dwarf.loc[UNW_PPC32_R4] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R4, 0);
	  c->dwarf.loc[UNW_PPC32_R5] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R5, 0);
	  c->dwarf.loc[UNW_PPC32_R6] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R6, 0);
	  c->dwarf.loc[UNW_PPC32_R7] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R7, 0);
	  c->dwarf.loc[UNW_PPC32_R8] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R8, 0);
	  c->dwarf.loc[UNW_PPC32_R9] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R9, 0);
	  c->dwarf.loc[UNW_PPC32_R10] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R10, 0);
	  c->dwarf.loc[UNW_PPC32_R11] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R11, 0);
	  c->dwarf.loc[UNW_PPC32_R12] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R12, 0);
	  c->dwarf.loc[UNW_PPC32_R13] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R13, 0);
	  c->dwarf.loc[UNW_PPC32_R14] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R14, 0);
	  c->dwarf.loc[UNW_PPC32_R15] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R15, 0);
	  c->dwarf.loc[UNW_PPC32_R16] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R16, 0);
	  c->dwarf.loc[UNW_PPC32_R17] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R17, 0);
	  c->dwarf.loc[UNW_PPC32_R18] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R18, 0);
	  c->dwarf.loc[UNW_PPC32_R19] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R19, 0);
	  c->dwarf.loc[UNW_PPC32_R20] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R20, 0);
	  c->dwarf.loc[UNW_PPC32_R21] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R21, 0);
	  c->dwarf.loc[UNW_PPC32_R22] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R22, 0);
	  c->dwarf.loc[UNW_PPC32_R23] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R23, 0);
	  c->dwarf.loc[UNW_PPC32_R24] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R24, 0);
	  c->dwarf.loc[UNW_PPC32_R25] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R25, 0);
	  c->dwarf.loc[UNW_PPC32_R26] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R26, 0);
	  c->dwarf.loc[UNW_PPC32_R27] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R27, 0);
	  c->dwarf.loc[UNW_PPC32_R28] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R28, 0);
	  c->dwarf.loc[UNW_PPC32_R29] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R29, 0);
	  c->dwarf.loc[UNW_PPC32_R30] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R30, 0);
	  c->dwarf.loc[UNW_PPC32_R31] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R31, 0);

	  c->dwarf.loc[UNW_PPC32_LR] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_LINK, 0);
	  c->dwarf.loc[UNW_PPC32_CTR] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_CTR, 0);

	  


	  c->dwarf.loc[UNW_PPC32_CCR] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_CCR, 0);
	  c->dwarf.loc[UNW_PPC32_XER] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_XER, 0);

	  c->dwarf.loc[UNW_PPC32_F0] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R0, 0);
	  c->dwarf.loc[UNW_PPC32_F1] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R1, 0);
	  c->dwarf.loc[UNW_PPC32_F2] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R2, 0);
	  c->dwarf.loc[UNW_PPC32_F3] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R3, 0);
	  c->dwarf.loc[UNW_PPC32_F4] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R4, 0);
	  c->dwarf.loc[UNW_PPC32_F5] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R5, 0);
	  c->dwarf.loc[UNW_PPC32_F6] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R6, 0);
	  c->dwarf.loc[UNW_PPC32_F7] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R7, 0);
	  c->dwarf.loc[UNW_PPC32_F8] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R8, 0);
	  c->dwarf.loc[UNW_PPC32_F9] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R9, 0);
	  c->dwarf.loc[UNW_PPC32_F10] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R10, 0);
	  c->dwarf.loc[UNW_PPC32_F11] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R11, 0);
	  c->dwarf.loc[UNW_PPC32_F12] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R12, 0);
	  c->dwarf.loc[UNW_PPC32_F13] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R13, 0);
	  c->dwarf.loc[UNW_PPC32_F14] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R14, 0);
	  c->dwarf.loc[UNW_PPC32_F15] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R15, 0);
	  c->dwarf.loc[UNW_PPC32_F16] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R16, 0);
	  c->dwarf.loc[UNW_PPC32_F17] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R17, 0);
	  c->dwarf.loc[UNW_PPC32_F18] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R18, 0);
	  c->dwarf.loc[UNW_PPC32_F19] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R19, 0);
	  c->dwarf.loc[UNW_PPC32_F20] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R20, 0);
	  c->dwarf.loc[UNW_PPC32_F21] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R21, 0);
	  c->dwarf.loc[UNW_PPC32_F22] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R22, 0);
	  c->dwarf.loc[UNW_PPC32_F23] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R23, 0);
	  c->dwarf.loc[UNW_PPC32_F24] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R24, 0);
	  c->dwarf.loc[UNW_PPC32_F25] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R25, 0);
	  c->dwarf.loc[UNW_PPC32_F26] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R26, 0);
	  c->dwarf.loc[UNW_PPC32_F27] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R27, 0);
	  c->dwarf.loc[UNW_PPC32_F28] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R28, 0);
	  c->dwarf.loc[UNW_PPC32_F29] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R29, 0);
	  c->dwarf.loc[UNW_PPC32_F30] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R30, 0);
	  c->dwarf.loc[UNW_PPC32_F31] =
	    DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R31, 0);

	  ret = 1;
	}
    }
  return ret;
}
