























#include "unwind_i.h"



PROTECTED int
unw_get_save_loc (unw_cursor_t *cursor, int reg, unw_save_loc_t *sloc)
{
  struct cursor *c = (struct cursor *) cursor;
  dwarf_loc_t loc;

  loc = DWARF_NULL_LOC;		

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

    default:
      break;
    }

  memset (sloc, 0, sizeof (*sloc));

  if (DWARF_IS_NULL_LOC (loc))
    {
      sloc->type = UNW_SLT_NONE;
      return 0;
    }

#if !defined(UNW_LOCAL_ONLY)
  if (DWARF_IS_REG_LOC (loc))
    {
      sloc->type = UNW_SLT_REG;
      sloc->u.regnum = DWARF_GET_LOC (loc);
    }
  else
#endif
    {
      sloc->type = UNW_SLT_MEMORY;
      sloc->u.addr = DWARF_GET_LOC (loc);
    }
  return 0;
}
