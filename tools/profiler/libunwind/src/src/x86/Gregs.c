
























#include "offsets.h"
#include "unwind_i.h"

HIDDEN dwarf_loc_t
x86_scratch_loc (struct cursor *c, unw_regnum_t reg)
{
  if (c->sigcontext_addr)
    return x86_get_scratch_loc (c, reg);
  else
    return DWARF_REG_LOC (&c->dwarf, reg);
}

HIDDEN int
tdep_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  dwarf_loc_t loc = DWARF_NULL_LOC;
  unsigned int mask;
  int arg_num;

  switch (reg)
    {

    case UNW_X86_EIP:
      if (write)
	c->dwarf.ip = *valp;		
      loc = c->dwarf.loc[EIP];
      break;

    case UNW_X86_CFA:
    case UNW_X86_ESP:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;

    case UNW_X86_EAX:
    case UNW_X86_EDX:
      arg_num = reg - UNW_X86_EAX;
      mask = (1 << arg_num);
      if (write)
	{
	  c->dwarf.eh_args[arg_num] = *valp;
	  c->dwarf.eh_valid_mask |= mask;
	  return 0;
	}
      else if ((c->dwarf.eh_valid_mask & mask) != 0)
	{
	  *valp = c->dwarf.eh_args[arg_num];
	  return 0;
	}
      else
	loc = c->dwarf.loc[(reg == UNW_X86_EAX) ? EAX : EDX];
      break;

    case UNW_X86_ECX: loc = c->dwarf.loc[ECX]; break;
    case UNW_X86_EBX: loc = c->dwarf.loc[EBX]; break;

    case UNW_X86_EBP: loc = c->dwarf.loc[EBP]; break;
    case UNW_X86_ESI: loc = c->dwarf.loc[ESI]; break;
    case UNW_X86_EDI: loc = c->dwarf.loc[EDI]; break;
    case UNW_X86_EFLAGS: loc = c->dwarf.loc[EFLAGS]; break;
    case UNW_X86_TRAPNO: loc = c->dwarf.loc[TRAPNO]; break;

    case UNW_X86_FCW:
    case UNW_X86_FSW:
    case UNW_X86_FTW:
    case UNW_X86_FOP:
    case UNW_X86_FCS:
    case UNW_X86_FIP:
    case UNW_X86_FEA:
    case UNW_X86_FDS:
    case UNW_X86_MXCSR:
    case UNW_X86_GS:
    case UNW_X86_FS:
    case UNW_X86_ES:
    case UNW_X86_DS:
    case UNW_X86_SS:
    case UNW_X86_CS:
    case UNW_X86_TSS:
    case UNW_X86_LDT:
      loc = x86_scratch_loc (c, reg);
      break;

    default:
      Debug (1, "bad register number %u\n", reg);
      return -UNW_EBADREG;
    }

  if (write)
    return dwarf_put (&c->dwarf, loc, *valp);
  else
    return dwarf_get (&c->dwarf, loc, valp);
}

HIDDEN int
tdep_access_fpreg (struct cursor *c, unw_regnum_t reg, unw_fpreg_t *valp,
		   int write)
{
  struct dwarf_loc loc = DWARF_NULL_LOC;

  switch (reg)
    {
    case UNW_X86_ST0:
      loc = c->dwarf.loc[ST0];
      break;

      
    case UNW_X86_ST1:
    case UNW_X86_ST2:
    case UNW_X86_ST3:
    case UNW_X86_ST4:
    case UNW_X86_ST5:
    case UNW_X86_ST6:
    case UNW_X86_ST7:
      
    case UNW_X86_XMM0:
    case UNW_X86_XMM1:
    case UNW_X86_XMM2:
    case UNW_X86_XMM3:
    case UNW_X86_XMM4:
    case UNW_X86_XMM5:
    case UNW_X86_XMM6:
    case UNW_X86_XMM7:
    case UNW_X86_XMM0_lo:
    case UNW_X86_XMM0_hi:
    case UNW_X86_XMM1_lo:
    case UNW_X86_XMM1_hi:
    case UNW_X86_XMM2_lo:
    case UNW_X86_XMM2_hi:
    case UNW_X86_XMM3_lo:
    case UNW_X86_XMM3_hi:
    case UNW_X86_XMM4_lo:
    case UNW_X86_XMM4_hi:
    case UNW_X86_XMM5_lo:
    case UNW_X86_XMM5_hi:
    case UNW_X86_XMM6_lo:
    case UNW_X86_XMM6_hi:
    case UNW_X86_XMM7_lo:
    case UNW_X86_XMM7_hi:
      loc = x86_scratch_loc (c, reg);
      break;

    default:
      Debug (1, "bad register number %u\n", reg);
      return -UNW_EBADREG;
    }

  if (write)
    return dwarf_putfp (&c->dwarf, loc, *valp);
  else
    return dwarf_getfp (&c->dwarf, loc, valp);
}
