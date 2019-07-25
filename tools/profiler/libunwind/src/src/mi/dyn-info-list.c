
























#include "libunwind_i.h"

HIDDEN unw_dyn_info_list_t _U_dyn_info_list;

PROTECTED unw_word_t
_U_dyn_info_list_addr (void)
{
  return (unw_word_t) (uintptr_t) &_U_dyn_info_list;
}
