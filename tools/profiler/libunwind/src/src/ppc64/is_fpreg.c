


























#include "libunwind_i.h"

PROTECTED int
unw_is_fpreg (int regnum)
{
  return (regnum >= UNW_PPC64_F0 && regnum <= UNW_PPC64_F31);
}
