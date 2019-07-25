


























#include <libunwind_i.h>

PROTECTED int
unw_is_signal_frame (unw_cursor_t * cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, w1, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  as->validate = 1;		
  arg = c->dwarf.as_arg;

  






  ip = c->dwarf.ip;
  if (ip == 0)
    return 0;

  



  a = unw_get_accessors (as);
  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip + 8, &w1, 0, arg)) < 0)
    return 0;
  w1 >>= 32;
  return (w0 == 0x38210080380000ac && w1 == 0x44000002);

}
