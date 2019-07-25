
























#include "_UPT_internal.h"

void
_UPT_put_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi, void *arg)
{
  if (!pi->unwind_info)
    return;
  free (pi->unwind_info);
  pi->unwind_info = NULL;
}
