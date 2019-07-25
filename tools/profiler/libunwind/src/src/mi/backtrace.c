
























#ifndef UNW_REMOTE_ONLY

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <libunwind_i.h>
#include <string.h>



static ALWAYS_INLINE int
slow_backtrace (void **buffer, int size, unw_context_t *uc)
{
  unw_cursor_t cursor;
  unw_word_t ip;
  int n = 0;

  if (unlikely (unw_init_local (&cursor, uc) < 0))
    return 0;

  while (unw_step (&cursor) > 0)
    {
      if (n >= size)
	return n;

      if (unw_get_reg (&cursor, UNW_REG_IP, &ip) < 0)
	return n;
      buffer[n++] = (void *) (uintptr_t) ip;
    }
  return n;
}

int
unw_backtrace (void **buffer, int size)
{
  unw_cursor_t cursor;
  unw_context_t uc;
  int n = size;

  tdep_getcontext_trace (&uc);

  if (unlikely (unw_init_local (&cursor, &uc) < 0))
    return 0;

  if (unlikely (tdep_trace (&cursor, buffer, &n) < 0))
    {
      unw_getcontext (&uc);
      return slow_backtrace (buffer, size, &uc);
    }

  return n;
}

extern int backtrace (void **buffer, int size)
  __attribute__((weak, alias("unw_backtrace")));

#endif 
