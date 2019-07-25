























#include "unwind_i.h"
#include <stdio.h>



PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  printf ("%s: implement me\n", __FUNCTION__);
  return -UNW_ENOINFO;
}
