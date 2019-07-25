
























#include "libunwind_i.h"

PROTECTED void
unw_destroy_addr_space (unw_addr_space_t as)
{
#ifndef UNW_LOCAL_ONLY
# if UNW_DEBUG
  memset (as, 0, sizeof (*as));
# endif
  free (as);
#endif
}
