


























#include "unwind_i.h"

HIDDEN int
tdep_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  struct dwarf_loc loc;

  switch (reg)
    {
    case UNW_TDEP_IP:
      if (write)
	{
	  c->dwarf.ip = *valp;	
	  if (c->dwarf.pi_valid && (*valp < c->dwarf.pi.start_ip
				    || *valp >= c->dwarf.pi.end_ip))
	    c->dwarf.pi_valid = 0;	
	}
      else
	*valp = c->dwarf.ip;
      return 0;

    case UNW_TDEP_SP:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;


    default:
      break;
    }

  
  if ((((unsigned) (reg - UNW_PPC64_F0)) <= 31) ||
      (((unsigned) (reg - UNW_PPC64_V0)) <= 31))
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

  if ((unsigned) (reg - UNW_PPC64_F0) < 32)
  {
    loc = c->dwarf.loc[reg];
    if (write)
      return dwarf_putfp (&c->dwarf, loc, *valp);
    else
      return dwarf_getfp (&c->dwarf, loc, valp);
  }
  else
  if ((unsigned) (reg - UNW_PPC64_V0) < 32)
  {
    loc = c->dwarf.loc[reg];
    if (write)
      return dwarf_putvr (&c->dwarf, loc, *valp);
    else
      return dwarf_getvr (&c->dwarf, loc, valp);
  }

  return -UNW_EBADREG;
}

