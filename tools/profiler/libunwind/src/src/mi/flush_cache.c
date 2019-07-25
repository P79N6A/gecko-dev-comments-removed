
























#include "libunwind_i.h"

PROTECTED void
unw_flush_cache (unw_addr_space_t as, unw_word_t lo, unw_word_t hi)
{
#if !UNW_TARGET_IA64
  struct unw_debug_frame_list *w = as->debug_frames;
#endif

  
  as->dyn_info_list_addr = 0;

#if !UNW_TARGET_IA64
  for (; w; w = w->next)
    {
      if (w->index)
        free (w->index);
      free (w->debug_frame);
    }
  as->debug_frames = NULL;
#endif

  




#ifdef HAVE_FETCH_AND_ADD1
  fetch_and_add1 (&as->cache_generation);
#else
# warning unw_flush_cache(): need a way to atomically increment an integer.
  ++as->cache_generation;
#endif
}
