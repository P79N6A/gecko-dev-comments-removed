























#include "unwind_i.h"
#include "ucontext_i.h"

HIDDEN void
tdep_stash_frame (struct dwarf_cursor *d, struct dwarf_reg_state *rs)
{
  struct cursor *c = (struct cursor *) dwarf_to_cursor (d);
  unw_tdep_frame_t *f = &c->frame_info;

  Debug (4, "ip=0x%lx cfa=0x%lx type %d cfa [where=%d val=%ld] cfaoff=%ld"
	 " ra=0x%lx rbp [where=%d val=%ld @0x%lx] rsp [where=%d val=%ld @0x%lx]\n",
	 d->ip, d->cfa, f->frame_type,
	 rs->reg[DWARF_CFA_REG_COLUMN].where,
	 rs->reg[DWARF_CFA_REG_COLUMN].val,
	 rs->reg[DWARF_CFA_OFF_COLUMN].val,
	 DWARF_GET_LOC(d->loc[d->ret_addr_column]),
	 rs->reg[RBP].where, rs->reg[RBP].val, DWARF_GET_LOC(d->loc[RBP]),
	 rs->reg[RSP].where, rs->reg[RSP].val, DWARF_GET_LOC(d->loc[RSP]));

  




  if (f->frame_type == UNW_X86_64_FRAME_OTHER
      && (rs->reg[DWARF_CFA_REG_COLUMN].where == DWARF_WHERE_REG)
      && (rs->reg[DWARF_CFA_REG_COLUMN].val == RBP
	  || rs->reg[DWARF_CFA_REG_COLUMN].val == RSP)
      && labs(rs->reg[DWARF_CFA_OFF_COLUMN].val) < (1 << 29)
      && DWARF_GET_LOC(d->loc[d->ret_addr_column]) == d->cfa-8
      && (rs->reg[RBP].where == DWARF_WHERE_UNDEF
	  || rs->reg[RBP].where == DWARF_WHERE_SAME
	  || (rs->reg[RBP].where == DWARF_WHERE_CFAREL
	      && labs(rs->reg[RBP].val) < (1 << 14)
	      && rs->reg[RBP].val+1 != 0))
      && (rs->reg[RSP].where == DWARF_WHERE_UNDEF
	  || rs->reg[RSP].where == DWARF_WHERE_SAME
	  || (rs->reg[RSP].where == DWARF_WHERE_CFAREL
	      && labs(rs->reg[RSP].val) < (1 << 14)
	      && rs->reg[RSP].val+1 != 0)))
  {
    
    f->frame_type = UNW_X86_64_FRAME_STANDARD;
    f->cfa_reg_rsp = (rs->reg[DWARF_CFA_REG_COLUMN].val == RSP);
    f->cfa_reg_offset = rs->reg[DWARF_CFA_OFF_COLUMN].val;
    if (rs->reg[RBP].where == DWARF_WHERE_CFAREL)
      f->rbp_cfa_offset = rs->reg[RBP].val;
    if (rs->reg[RSP].where == DWARF_WHERE_CFAREL)
      f->rsp_cfa_offset = rs->reg[RSP].val;
    Debug (4, " standard frame\n");
  }

  
  else if (f->frame_type == UNW_X86_64_FRAME_SIGRETURN)
  {
    



#ifndef NDEBUG
    const unw_word_t uc = c->sigcontext_addr;

    assert (DWARF_GET_LOC(d->loc[RIP]) - uc == UC_MCONTEXT_GREGS_RIP);
    assert (DWARF_GET_LOC(d->loc[RBP]) - uc == UC_MCONTEXT_GREGS_RBP);
    assert (DWARF_GET_LOC(d->loc[RSP]) - uc == UC_MCONTEXT_GREGS_RSP);
#endif

    Debug (4, " sigreturn frame\n");
  }

  
  else
    Debug (4, " unusual frame\n");
}
