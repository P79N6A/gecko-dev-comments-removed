
























#include "unwind_i.h"

PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
#ifdef __linux__ 
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, w1, w2, w3, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors (as);
  arg = c->dwarf.as_arg;

  











  ip = c->dwarf.ip;
  if (!ip)
    return 0;
  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip + 4, &w1, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip + 8, &w2, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip + 12, &w3, 0, arg)) < 0)
    {
      Debug (1, "failed to read sigreturn code (ret=%d)\n", ret);
      return ret;
    }
  ret = ((w0 == 0x34190000 || w0 == 0x34190002)
	 && w1 == 0x3414015a && w2 == 0xe4008200 && w3 == 0x08000240);
  Debug (1, "(cursor=%p, ip=0x%08lx) -> %d\n", c, (unsigned) ip, ret);
  return ret;
#else
  printf ("%s: implement me\n", __FUNCTION__);
#endif
  return -UNW_ENOINFO;
}
