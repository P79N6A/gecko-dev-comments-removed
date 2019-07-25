
























#include <libunwind.h>
#include <setjmp.h>
#include <stdlib.h>

#include "jmpbuf.h"

int
sigsetjmp (sigjmp_buf env, int savemask)
{
  unw_word_t *wp = (unw_word_t *) env;

  


  wp[JB_SP] = (unw_word_t) __builtin_frame_address (0);
  wp[JB_RP] = (unw_word_t) __builtin_return_address (0);
  wp[JB_MASK_SAVED] = savemask;

  

  if (savemask
      && sigprocmask (SIG_BLOCK, NULL, (sigset_t *) (wp + JB_MASK)) < 0)
    abort ();
  return 0;
}
