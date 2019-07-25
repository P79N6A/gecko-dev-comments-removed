

























#include <stdlib.h>

#include "unwind_i.h"

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
mips_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  return -UNW_EINVAL;
}

#endif 

PROTECTED int
unw_resume (unw_cursor_t *cursor)
{
  return -UNW_EINVAL;
}
