






























#include <stdio.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

extern int test_generic (void);

int verbose;

#ifdef UNW_REMOTE_ONLY

int
test_local (void)
{
  return 0;
}

#else 

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
    (void *) &unw_get_proc_name,
    (void *) &_U_dyn_register,
    (void *) &_U_dyn_cancel
  };

int
test_local (void)
{
  unw_context_t uc;
  unw_cursor_t c;

  if (verbose)
    printf (__FILE__": funcs[0]=%p\n", funcs[0]);

  unw_getcontext (&uc);
  unw_init_local (&c, &uc);
  unw_init_remote (&c, unw_local_addr_space, &uc);
  return unw_step (&c);
}

#endif 

int
main (int argc, char **argv)
{
  if (argc > 1)
    verbose = 1;

  if (test_local () < 0)
    return -1;
  if (test_generic () < 0)
    return -1;
  return 0;
}
