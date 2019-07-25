

































#include <stdio.h>
#include <string.h>

#include <libunwind.h>

int errors;

#define panic(args...)					\
	{ ++errors; fprintf (stderr, args); return -1; }

static int
find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pip,
		int need_unwind_info, void *arg)
{
  return -UNW_ESTOPUNWIND;
}

static int
access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *valp,
	    int write, void *arg)
{
  if (!write)
    *valp = 0;
  return 0;
}

static int
access_reg (unw_addr_space_t as, unw_regnum_t regnum, unw_word_t *valp,
	    int write, void *arg)
{
  if (!write)
    *valp = 32;
  return 0;
}

static int
access_fpreg (unw_addr_space_t as, unw_regnum_t regnum, unw_fpreg_t *valp,
	      int write, void *arg)
{
  if (!write)
    memset (valp, 0, sizeof (*valp));
  return 0;
}

static int
get_dyn_info_list_addr (unw_addr_space_t as, unw_word_t *dilap, void *arg)
{
  return -UNW_ENOINFO;
}

static void
put_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi, void *arg)
{
  ++errors;
  fprintf (stderr, "%s() got called!\n", __FUNCTION__);
}

static int
resume (unw_addr_space_t as, unw_cursor_t *reg, void *arg)
{
  panic ("%s() got called!\n", __FUNCTION__);
}

static int
get_proc_name (unw_addr_space_t as, unw_word_t ip, char *buf, size_t buf_len,
	       unw_word_t *offp, void *arg)
{
  panic ("%s() got called!\n", __FUNCTION__);
}

int
main (int argc, char **argv)
{
  unw_accessors_t acc;
  unw_addr_space_t as;
  int ret, verbose = 0;
  unw_cursor_t c;

  if (argc > 1 && strcmp (argv[1], "-v") == 0)
    verbose = 1;

  memset (&acc, 0, sizeof (acc));
  acc.find_proc_info = find_proc_info;
  acc.put_unwind_info = put_unwind_info;
  acc.get_dyn_info_list_addr = get_dyn_info_list_addr;
  acc.access_mem = access_mem;
  acc.access_reg = access_reg;
  acc.access_fpreg = access_fpreg;
  acc.resume = resume;
  acc.get_proc_name = get_proc_name;

  as = unw_create_addr_space (&acc, 0);
  if (!as)
    panic ("unw_create_addr_space() failed\n");

  unw_set_caching_policy (as, UNW_CACHE_GLOBAL);

  ret = unw_init_remote (&c, as, NULL);
  if (ret < 0)
    panic ("unw_init_remote() returned %d instead of 0\n", ret);

  ret = unw_step (&c);
  if (ret != -UNW_ESTOPUNWIND)
    panic ("First call to unw_step() returned %d instead of %d\n",
	   ret, -UNW_ESTOPUNWIND);

  ret = unw_step (&c);
  if (ret != -UNW_ESTOPUNWIND)
    panic ("Second call to unw_step() returned %d instead of %d\n",
	   ret, -UNW_ESTOPUNWIND);

  if (verbose)
    printf ("SUCCESS\n");
  return 0;
}
