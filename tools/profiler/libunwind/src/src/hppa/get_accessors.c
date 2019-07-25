
























#include "unwind_i.h"

PROTECTED unw_accessors_t *
unw_get_accessors (unw_addr_space_t as)
{
  if (hppa_needs_initialization)
    {
      hppa_needs_initialization = 0;
      hppa_init ();
    }
  return &as->acc;
}
