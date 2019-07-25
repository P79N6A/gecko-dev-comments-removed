























#include "unwind_i.h"

static const char *regname[] =
  {
    
    "$0",  "$1",  "$2",  "$3",  "$4",  "$5",  "$6",  "$7",
    
    "$8",  "$9",  "$10", "$11",  "$12",  "$13",  "$14",  "$15",
    
    "$16",  "$17",  "$18", "$19",  "$20",  "$21",  "$22",  "$23",
    
    "$24",  "$25",  "$26", "$27",  "$28",  "$29",  "$30",  "$31",
  };

PROTECTED const char *
unw_regname (unw_regnum_t reg)
{
  if (reg < (unw_regnum_t) ARRAY_SIZE (regname))
    return regname[reg];
  else
    return "???";
}
