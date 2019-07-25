
























#include <stdlib.h>

#include "unwind_i.h"

#if defined(_LITTLE_ENDIAN) && !defined(__LITTLE_ENDIAN)
#define __LITTLE_ENDIAN _LITTLE_ENDIAN
#endif

PROTECTED unw_addr_space_t
unw_create_addr_space (unw_accessors_t *a, int byte_order)
{
#ifdef UNW_LOCAL_ONLY
  return NULL;
#else
  unw_addr_space_t as = malloc (sizeof (*as));

  if (!as)
    return NULL;

  memset (as, 0, sizeof (*as));

  as->acc = *a;

  


  if (byte_order != 0 && byte_order != __LITTLE_ENDIAN)
    return NULL;
  return as;
#endif
}
