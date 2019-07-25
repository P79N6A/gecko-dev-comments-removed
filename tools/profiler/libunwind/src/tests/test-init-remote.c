





























#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libunwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

int verbose;

static int
do_backtrace (void)
{
  char buf[512], name[256];
  unw_word_t ip, sp, off;
  unw_cursor_t cursor;
  unw_context_t uc;
  int ret;

  unw_getcontext (&uc);
  if (unw_init_remote (&cursor, unw_local_addr_space, &uc) < 0)
    panic ("unw_init_remote failed!\n");

  do
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      unw_get_reg (&cursor, UNW_REG_SP, &sp);
      buf[0] = '\0';
      if (unw_get_proc_name (&cursor, name, sizeof (name), &off) == 0)
	{
	  if (off)
	    snprintf (buf, sizeof (buf), "<%s+0x%lx>", name, (long) off);
	  else
	    snprintf (buf, sizeof (buf), "<%s>", name);
	}
      if (verbose)
	printf ("%016lx %-32s (sp=%016lx)\n", (long) ip, buf, (long) sp);

      ret = unw_step (&cursor);
      if (ret < 0)
	{
	  unw_get_reg (&cursor, UNW_REG_IP, &ip);
	  printf ("FAILURE: unw_step() returned %d for ip=%lx\n",
		  ret, (long) ip);
	  return -1;
	}
    }
  while (ret > 0);

  return 0;
}

static int
foo (void)
{
  return do_backtrace ();
}

int
main (int argc, char **argv)
{
  verbose = (argc > 1);

  if (verbose)
    printf ("Normal backtrace:\n");
  return foo ();
}
