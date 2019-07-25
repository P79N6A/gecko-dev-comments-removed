


























#include <stdio.h>

#include <libunwind.h>

extern int verbose;

static void *funcs[] =
  {
    (void *) &unw_get_reg,
    (void *) &unw_get_fpreg,
    (void *) &unw_set_reg,
    (void *) &unw_set_fpreg,
    (void *) &unw_resume,
    (void *) &unw_create_addr_space,
    (void *) &unw_destroy_addr_space,
    (void *) &unw_get_accessors,
    (void *) &unw_flush_cache,
    (void *) &unw_set_caching_policy,
    (void *) &unw_regname,
    (void *) &unw_get_proc_info,
    (void *) &unw_get_save_loc,
    (void *) &unw_is_signal_frame,
    (void *) &unw_get_proc_name
  };

int
test_generic (void)
{
  if (verbose)
    printf (__FILE__": funcs[0]=%p\n", funcs[0]);

#ifndef UNW_REMOTE_ONLY
  {
    unw_context_t uc;
    unw_cursor_t c;

    unw_getcontext (&uc);
    unw_init_local (&c, &uc);
    unw_init_remote (&c, unw_local_addr_space, &uc);

    return unw_step (&c);
  }
#else
  return 0;
#endif
}
