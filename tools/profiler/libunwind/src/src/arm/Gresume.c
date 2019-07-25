
























#include "unwind_i.h"
#include "offsets.h"

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
arm_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
#ifdef __linux__
  struct cursor *c = (struct cursor *) cursor;
  unw_tdep_context_t *uc = c->dwarf.as_arg;

  if (c->sigcontext_format == ARM_SCF_NONE)
    {
      

      unsigned long regs[10];
      regs[0] = uc->regs[4];
      regs[1] = uc->regs[5];
      regs[2] = uc->regs[6];
      regs[3] = uc->regs[7];
      regs[4] = uc->regs[8];
      regs[5] = uc->regs[9];
      regs[6] = uc->regs[10];
      regs[7] = uc->regs[11]; 
      regs[8] = uc->regs[13]; 
      regs[9] = uc->regs[14]; 

      asm __volatile__ (
	"ldmia %0, {r4-r12, lr}\n"
	"mov sp, r12\n"
	"bx lr\n"
	: : "r" (regs)
      );
    }
  else
    {
      

      struct sigcontext *sc = (struct sigcontext *) c->sigcontext_addr;
      sc->arm_r0 = uc->regs[0];
      sc->arm_r1 = uc->regs[1];
      sc->arm_r2 = uc->regs[2];
      sc->arm_r3 = uc->regs[3];
      sc->arm_r4 = uc->regs[4];
      sc->arm_r5 = uc->regs[5];
      sc->arm_r6 = uc->regs[6];
      sc->arm_r7 = uc->regs[7];
      sc->arm_r8 = uc->regs[8];
      sc->arm_r9 = uc->regs[9];
      sc->arm_r10 = uc->regs[10];
      sc->arm_fp = uc->regs[11]; 
      sc->arm_ip = uc->regs[12]; 
      sc->arm_sp = uc->regs[13]; 
      sc->arm_lr = uc->regs[14]; 
      sc->arm_pc = uc->regs[15]; 
      
      sc->arm_cpsr &= 0xf9ff03ffUL;

      

      asm __volatile__ (
	"mov sp, %0\n"
	"bx %1\n"
	: : "r" (c->sigcontext_sp), "r" (c->sigcontext_pc)
      );
   }
#else
  printf ("%s: implement me\n", __FUNCTION__);
#endif
  return -UNW_EINVAL;
}

#endif 

static inline void
establish_machine_state (struct cursor *c)
{
  unw_addr_space_t as = c->dwarf.as;
  void *arg = c->dwarf.as_arg;
  unw_fpreg_t fpval;
  unw_word_t val;
  int reg;

  Debug (8, "copying out cursor state\n");

  for (reg = 0; reg <= UNW_REG_LAST; ++reg)
    {
      Debug (16, "copying %s %d\n", unw_regname (reg), reg);
      if (unw_is_fpreg (reg))
	{
	  if (tdep_access_fpreg (c, reg, &fpval, 0) >= 0)
	    as->acc.access_fpreg (as, reg, &fpval, 1, arg);
	}
      else
	{
	  if (tdep_access_reg (c, reg, &val, 0) >= 0)
	    as->acc.access_reg (as, reg, &val, 1, arg);
	}
    }
}

PROTECTED int
unw_resume (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;

  Debug (1, "(cursor=%p)\n", c);

  if (!c->dwarf.ip)
    {
      

      Debug (1, "refusing to resume execution at address 0\n");
      return -UNW_EINVAL;
    }

  establish_machine_state (c);

  return (*c->dwarf.as->acc.resume) (c->dwarf.as, (unw_cursor_t *) c,
				     c->dwarf.as_arg);
}
