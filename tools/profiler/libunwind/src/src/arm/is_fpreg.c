























#include "libunwind_i.h"




PROTECTED int
unw_is_fpreg (int regnum)
{
  return ((regnum >= UNW_ARM_S0 && regnum <= UNW_ARM_S31)
	  || (regnum >= UNW_ARM_F0 && regnum <= UNW_ARM_F7)
	  || (regnum >= UNW_ARM_wCGR0 && regnum <= UNW_ARM_wCGR7)
	  || (regnum >= UNW_ARM_wR0 && regnum <= UNW_ARM_wR15)
	  || (regnum >= UNW_ARM_wC0 && regnum <= UNW_ARM_wC7)
	  || (regnum >= UNW_ARM_D0 && regnum <= UNW_ARM_D31));
}
