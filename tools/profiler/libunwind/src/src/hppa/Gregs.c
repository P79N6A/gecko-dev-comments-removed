
























#include "unwind_i.h"

HIDDEN int
tdep_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  struct dwarf_loc loc;

  switch (reg)
    {
    case UNW_HPPA_IP:
      if (write)
	c->dwarf.ip = *valp;		
      if (c->dwarf.pi_valid && (*valp < c->dwarf.pi.start_ip
				|| *valp >= c->dwarf.pi.end_ip))
	c->dwarf.pi_valid = 0;		
      break;

    case UNW_HPPA_CFA:
    case UNW_HPPA_SP:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;

      
    case UNW_HPPA_EH0: reg = UNW_HPPA_GR + 20; break;
    case UNW_HPPA_EH1: reg = UNW_HPPA_GR + 21; break;
    case UNW_HPPA_EH2: reg = UNW_HPPA_GR + 22; break;
    case UNW_HPPA_EH3: reg = UNW_HPPA_GR + 31; break;

    default:
      break;
    }

  if ((unsigned) (reg - UNW_HPPA_GR) >= 32)
    return -UNW_EBADREG;

  loc = c->dwarf.loc[reg];

  if (write)
    return dwarf_put (&c->dwarf, loc, *valp);
  else
    return dwarf_get (&c->dwarf, loc, valp);
}

HIDDEN int
tdep_access_fpreg (struct cursor *c, unw_regnum_t reg, unw_fpreg_t *valp,
		   int write)
{
  struct dwarf_loc loc;

  if ((unsigned) (reg - UNW_HPPA_FR) >= 32)
    return -UNW_EBADREG;

  loc = c->dwarf.loc[reg];

  if (write)
    return dwarf_putfp (&c->dwarf, loc, *valp);
  else
    return dwarf_getfp (&c->dwarf, loc, valp);
}
