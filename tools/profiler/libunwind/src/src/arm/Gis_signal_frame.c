
























#include <stdio.h>
#include "unwind_i.h"

#define __linux__
#ifdef __linux__
#define ARM_NR_sigreturn 119
#define ARM_NR_rt_sigreturn 173
#define ARM_NR_OABI_SYSCALL_BASE 0x900000


#define MOV_R7_SIGRETURN (0xe3a07000UL | ARM_NR_sigreturn)
#define MOV_R7_RT_SIGRETURN (0xe3a07000UL | ARM_NR_rt_sigreturn)


#define ARM_SIGRETURN \
  (0xef000000UL | ARM_NR_sigreturn | ARM_NR_OABI_SYSCALL_BASE)
#define ARM_RT_SIGRETURN \
  (0xef000000UL | ARM_NR_rt_sigreturn | ARM_NR_OABI_SYSCALL_BASE)


#define THUMB_SIGRETURN (0xdf00UL << 16 | 0x2700 | ARM_NR_sigreturn)
#define THUMB_RT_SIGRETURN (0xdf00UL << 16 | 0x2700 | ARM_NR_rt_sigreturn)
#endif 



PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
#ifdef __linux__
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors (as);
  arg = c->dwarf.as_arg;

  ip = c->dwarf.ip;

  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0)
    return ret;

  
  if (w0 == MOV_R7_SIGRETURN || w0 == ARM_SIGRETURN || w0 == THUMB_SIGRETURN)
    return 1;
  
  else if (w0 == MOV_R7_RT_SIGRETURN || w0 == ARM_RT_SIGRETURN
           || w0 == THUMB_RT_SIGRETURN)
    return 2;

  return 0;
#else
  printf ("%s: implement me\n", __FUNCTION__);
  return -UNW_ENOINFO;
#endif
}
