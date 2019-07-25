























#include "unwind_i.h"

HIDDEN int
tdep_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  dwarf_loc_t loc = DWARF_NULL_LOC;
  
  switch (reg)
    {
    case UNW_ARM_R0:
    case UNW_ARM_R1:
    case UNW_ARM_R2:
    case UNW_ARM_R3:
    case UNW_ARM_R4:
    case UNW_ARM_R5:
    case UNW_ARM_R6:
    case UNW_ARM_R7:
    case UNW_ARM_R8:
    case UNW_ARM_R9:
    case UNW_ARM_R10:
    case UNW_ARM_R11:
    case UNW_ARM_R12:
    case UNW_ARM_R14:
    case UNW_ARM_R15:
      loc = c->dwarf.loc[reg - UNW_ARM_R0];
      break;

    case UNW_ARM_R13:
    case UNW_ARM_CFA:
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
