























#include "unwind_i.h"

static inline int
common_init (struct cursor *c, unsigned use_prev_instr)
{
  int ret, i;

  c->dwarf.loc[UNW_ARM_R0] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R0);
  c->dwarf.loc[UNW_ARM_R1] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R1);
  c->dwarf.loc[UNW_ARM_R2] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R2);
  c->dwarf.loc[UNW_ARM_R3] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R3);
  c->dwarf.loc[UNW_ARM_R4] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R4);
  c->dwarf.loc[UNW_ARM_R5] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R5);
  c->dwarf.loc[UNW_ARM_R6] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R6);
  c->dwarf.loc[UNW_ARM_R7] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R7);
  c->dwarf.loc[UNW_ARM_R8] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R8);
  c->dwarf.loc[UNW_ARM_R9] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R9);
  c->dwarf.loc[UNW_ARM_R10] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R10);
  c->dwarf.loc[UNW_ARM_R11] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R11);
  c->dwarf.loc[UNW_ARM_R12] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R12);
  c->dwarf.loc[UNW_ARM_R13] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R13);
  c->dwarf.loc[UNW_ARM_R14] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R14);
  c->dwarf.loc[UNW_ARM_R15] = DWARF_REG_LOC (&c->dwarf, UNW_ARM_R15);
  for (i = UNW_ARM_R15 + 1; i < DWARF_NUM_PRESERVED_REGS; ++i)
    c->dwarf.loc[i] = DWARF_NULL_LOC;

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_ARM_R15], &c->dwarf.ip);
  if (ret < 0)
    return ret;

  
  ret = dwarf_get (&c->dwarf, DWARF_REG_LOC (&c->dwarf, UNW_ARM_R13),
		   &c->dwarf.cfa);
  if (ret < 0)
    return ret;

  c->sigcontext_format = ARM_SCF_NONE;
  c->sigcontext_addr = 0;
  c->sigcontext_sp = 0;
  c->sigcontext_pc = 0;

  

  c->dwarf.args_size = 0;
  c->dwarf.ret_addr_column = 0;
  c->dwarf.stash_frames = 0;
  c->dwarf.use_prev_instr = use_prev_instr;
  c->dwarf.pi_valid = 0;
  c->dwarf.pi_is_dynamic = 0;
  c->dwarf.hint = 0;
  c->dwarf.prev_rs = 0;

  return 0;
}
