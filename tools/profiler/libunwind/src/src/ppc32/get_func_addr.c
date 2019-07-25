


























#include "unwind_i.h"

int
tdep_get_func_addr (unw_addr_space_t as, unw_word_t symbol_val_addr,
		    unw_word_t *real_func_addr)
{
  *real_func_addr = symbol_val_addr;
  return 0;
}
