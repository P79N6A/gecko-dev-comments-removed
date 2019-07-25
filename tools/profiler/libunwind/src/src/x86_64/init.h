


























#include "unwind_i.h"


#if defined UNW_LOCAL_ONLY && defined __linux
# define REG_INIT_LOC(c, rlc, ruc) \
    DWARF_LOC ((unw_word_t) &c->uc->uc_mcontext.gregs[REG_ ## ruc], 0)

#elif defined UNW_LOCAL_ONLY && defined __FreeBSD__
# define REG_INIT_LOC(c, rlc, ruc) \
    DWARF_LOC ((unw_word_t) &c->uc->uc_mcontext.mc_ ## rlc, 0)

#else
# define REG_INIT_LOC(c, rlc, ruc) \
    DWARF_REG_LOC (&c->dwarf, UNW_X86_64_ ## ruc)
#endif

static inline int
common_init (struct cursor *c, unsigned use_prev_instr)
{
  int ret;

  c->dwarf.loc[RAX] = REG_INIT_LOC(c, rax, RAX);
  c->dwarf.loc[RDX] = REG_INIT_LOC(c, rdx, RDX);
  c->dwarf.loc[RCX] = REG_INIT_LOC(c, rcx, RCX);
  c->dwarf.loc[RBX] = REG_INIT_LOC(c, rbx, RBX);
  c->dwarf.loc[RSI] = REG_INIT_LOC(c, rsi, RSI);
  c->dwarf.loc[RDI] = REG_INIT_LOC(c, rdi, RDI);
  c->dwarf.loc[RBP] = REG_INIT_LOC(c, rbp, RBP);
  c->dwarf.loc[RSP] = REG_INIT_LOC(c, rsp, RSP);
  c->dwarf.loc[R8]  = REG_INIT_LOC(c, r8,  R8);
  c->dwarf.loc[R9]  = REG_INIT_LOC(c, r9,  R9);
  c->dwarf.loc[R10] = REG_INIT_LOC(c, r10, R10);
  c->dwarf.loc[R11] = REG_INIT_LOC(c, r11, R11);
  c->dwarf.loc[R12] = REG_INIT_LOC(c, r12, R12);
  c->dwarf.loc[R13] = REG_INIT_LOC(c, r13, R13);
  c->dwarf.loc[R14] = REG_INIT_LOC(c, r14, R14);
  c->dwarf.loc[R15] = REG_INIT_LOC(c, r15, R15);
  c->dwarf.loc[RIP] = REG_INIT_LOC(c, rip, RIP);

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);
  if (ret < 0)
    return ret;

  ret = dwarf_get (&c->dwarf, DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RSP),
		   &c->dwarf.cfa);
  if (ret < 0)
    return ret;

  c->sigcontext_format = X86_64_SCF_NONE;
  c->sigcontext_addr = 0;

  c->dwarf.args_size = 0;
  c->dwarf.ret_addr_column = RIP;
  c->dwarf.stash_frames = 0;
  c->dwarf.use_prev_instr = use_prev_instr;
  c->dwarf.pi_valid = 0;
  c->dwarf.pi_is_dynamic = 0;
  c->dwarf.hint = 0;
  c->dwarf.prev_rs = 0;

  return 0;
}
