


























#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libunwind.h>

#define panic(args...)							  \
	do { printf (args); ++nerrors; } while (0)

static int verbose;
static int nerrors;

extern void test_func (void (*) (void));

void
checker (void)
{
  unw_fpreg_t fpval;
  unw_context_t uc;
  unw_cursor_t c;
  int ret;

  fpval.raw.bits[0] = 100;
  fpval.raw.bits[1] = 101;

  unw_getcontext (&uc);

  if ((ret = unw_init_local (&c, &uc)) < 0)
    panic ("%s: unw_init_local (ret=%d)\n", __FUNCTION__, ret);

  if ((ret = unw_step (&c)) < 0)
    panic ("%s: unw_step (ret=%d)\n", __FUNCTION__, ret);

  if ((ret = unw_step (&c)) < 0)
    panic ("%s: unw_step (ret=%d)\n", __FUNCTION__, ret);

  if ((ret = unw_set_reg (&c, UNW_IA64_IP, 99)) != -UNW_EREADONLYREG)
    panic ("%s: unw_set_reg (ip) returned %d instead of %d\n",
	   __FUNCTION__, ret, -UNW_EREADONLYREG);
  if ((ret = unw_set_reg (&c, UNW_IA64_AR_LC, 99)) != -UNW_EREADONLYREG)
    panic ("%s: unw_set_reg (ar.lc) returned %d instead of %d\n",
	   __FUNCTION__, ret, -UNW_EREADONLYREG);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    verbose = 1;

  test_func (checker);

  if (nerrors > 0)
    {
      fprintf (stderr, "FAILURE: detected %d errors\n", nerrors);
      exit (-1);
    }
  if (verbose)
    printf ("SUCCESS.\n");
  return 0;
}
