


























#include "unwind_i.h"

static inline int
common_init_ppc64 (struct cursor *c, unsigned use_prev_instr)
{
  int ret;
  int i;

  for (i = UNW_PPC64_R0; i <= UNW_PPC64_R31; i++) {
    c->dwarf.loc[i] = DWARF_REG_LOC (&c->dwarf, i);
  }
  for (i = UNW_PPC64_F0; i <= UNW_PPC64_F31; i++) {
    c->dwarf.loc[i] = DWARF_FPREG_LOC (&c->dwarf, i);
  }
  for (i = UNW_PPC64_V0; i <= UNW_PPC64_V31; i++) {
    c->dwarf.loc[i] = DWARF_VREG_LOC (&c->dwarf, i);
  }

  for (i = UNW_PPC64_CR0; i <= UNW_PPC64_CR7; i++) {
    c->dwarf.loc[i] = DWARF_REG_LOC (&c->dwarf, i);
  }
  c->dwarf.loc[UNW_PPC64_ARG_POINTER] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_ARG_POINTER);
  c->dwarf.loc[UNW_PPC64_CTR] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_CTR);
  c->dwarf.loc[UNW_PPC64_VSCR] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_VSCR);

  c->dwarf.loc[UNW_PPC64_XER] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_XER);
  c->dwarf.loc[UNW_PPC64_LR] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_LR);
  c->dwarf.loc[UNW_PPC64_VRSAVE] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_VRSAVE);
  c->dwarf.loc[UNW_PPC64_SPEFSCR] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_SPEFSCR);
  c->dwarf.loc[UNW_PPC64_SPE_ACC] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_SPE_ACC);

  c->dwarf.loc[UNW_PPC64_NIP] = DWARF_REG_LOC (&c->dwarf, UNW_PPC64_NIP);

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_PPC64_NIP], &c->dwarf.ip);
  if (ret < 0)
    return ret;

  ret = dwarf_get (&c->dwarf, DWARF_REG_LOC (&c->dwarf, UNW_PPC64_R1),
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
