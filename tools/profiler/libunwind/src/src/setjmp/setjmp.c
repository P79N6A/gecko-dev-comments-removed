
























#include <libunwind.h>
#include <setjmp.h>

#include "jmpbuf.h"








int
setjmp (env)
     jmp_buf env;
{
  void **wp = (void **) env;

  

  wp[JB_SP] = __builtin_frame_address (0);
  wp[JB_RP] = (void *) __builtin_return_address (0);
  return 0;
}
