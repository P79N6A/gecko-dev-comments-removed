


























#include "unwind_i.h"



static inline int
common_init_ppc32 (struct cursor *c, unsigned use_prev_instr)
{
  int ret;
  int i;

  for (i = UNW_PPC32_R0; i <= UNW_PPC32_R31; i++) {
    c->dwarf.loc[i] = DWARF_REG_LOC (&c->dwarf, i);
  }
  for (i = UNW_PPC32_F0; i <= UNW_PPC32_F31; i++) {
    c->dwarf.loc[i] = DWARF_FPREG_LOC (&c->dwarf, i);
  }

  c->dwarf.loc[UNW_PPC32_CTR] = DWARF_REG_LOC (&c->dwarf, UNW_PPC32_CTR);
  c->dwarf.loc[UNW_PPC32_XER] = DWARF_REG_LOC (&c->dwarf, UNW_PPC32_XER);
  c->dwarf.loc[UNW_PPC32_CCR] = DWARF_REG_LOC (&c->dwarf, UNW_PPC32_CCR);
  c->dwarf.loc[UNW_PPC32_LR] = DWARF_REG_LOC (&c->dwarf, UNW_PPC32_LR);
  c->dwarf.loc[UNW_PPC32_FPSCR] = DWARF_REG_LOC (&c->dwarf, UNW_PPC32_FPSCR);

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_PPC32_LR], &c->dwarf.ip);
  if (ret < 0)
    return ret;

  ret = dwarf_get (&c->dwarf, DWARF_REG_LOC (&c->dwarf, UNW_PPC32_R1),
		   &c->dwarf.cfa);
  if (ret < 0)
    return ret;

  c->sigcontext_format = PPC_SCF_NONE;
  c->sigcontext_addr = 0;

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
