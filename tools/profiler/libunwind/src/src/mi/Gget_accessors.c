
























#include "libunwind_i.h"

PROTECTED unw_accessors_t *
unw_get_accessors (unw_addr_space_t as)
{
  if (tdep_needs_initialization)
    tdep_init ();
  return &as->acc;
}
