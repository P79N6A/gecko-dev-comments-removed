
























#include <stdlib.h>

#include "unwind_i.h"

PROTECTED unw_addr_space_t
unw_create_addr_space (unw_accessors_t *a, int byte_order)
{
#ifdef UNW_LOCAL_ONLY
  return NULL;
#else
  unw_addr_space_t as;

  



  if (byte_order != 0
      && byte_order != __LITTLE_ENDIAN
      && byte_order != __BIG_ENDIAN)
    return NULL;

  as = malloc (sizeof (*as));

  if (!as)
    return NULL;

  memset (as, 0, sizeof (*as));

  as->acc = *a;

  if (byte_order == 0)
    
    as->big_endian = (__BYTE_ORDER == __BIG_ENDIAN);
  else
    as->big_endian = (byte_order == __BIG_ENDIAN);
  return as;
#endif
}
