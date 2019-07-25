
























#include "libunwind_i.h"

void
_U_dyn_cancel (unw_dyn_info_t *di)
{
  mutex_lock (&_U_dyn_info_list_lock);
  {
    ++_U_dyn_info_list.generation;

    if (di->prev)
      di->prev->next = di->next;
    else
      _U_dyn_info_list.first = di->next;

    if (di->next)
      di->next->prev = di->prev;
  }
  mutex_unlock (&_U_dyn_info_list_lock);

  di->next = di->prev = NULL;
}
