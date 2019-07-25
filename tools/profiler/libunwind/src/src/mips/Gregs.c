























#include "unwind_i.h"



HIDDEN int
tdep_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  dwarf_loc_t loc = DWARF_NULL_LOC;
  
  switch (reg)
    {
    case UNW_MIPS_R0:
    case UNW_MIPS_R1:
    case UNW_MIPS_R2:
    case UNW_MIPS_R3:
    case UNW_MIPS_R4:
    case UNW_MIPS_R5:
    case UNW_MIPS_R6:
    case UNW_MIPS_R7:
    case UNW_MIPS_R8:
    case UNW_MIPS_R9:
    case UNW_MIPS_R10:
    case UNW_MIPS_R11:
    case UNW_MIPS_R12:
    case UNW_MIPS_R13:
    case UNW_MIPS_R14:
    case UNW_MIPS_R15:
    case UNW_MIPS_R16:
    case UNW_MIPS_R17:
    case UNW_MIPS_R18:
    case UNW_MIPS_R19:
    case UNW_MIPS_R20:
    case UNW_MIPS_R21:
    case UNW_MIPS_R22:
    case UNW_MIPS_R23:
    case UNW_MIPS_R24:
    case UNW_MIPS_R25:
    case UNW_MIPS_R26:
    case UNW_MIPS_R27:
    case UNW_MIPS_R28:
    case UNW_MIPS_R29:
    case UNW_MIPS_R30:
    case UNW_MIPS_R31:
      loc = c->dwarf.loc[reg - UNW_MIPS_R0];
      break;

    case UNW_MIPS_CFA:
      if (write)
        return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;

    

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
  Debug (1, "bad register number %u\n", reg);
  return -UNW_EBADREG;
}
