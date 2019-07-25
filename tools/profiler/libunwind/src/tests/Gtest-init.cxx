



























#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libunwind.h>

int verbose, errors;

#define panic(args...)					\
	{ ++errors; fprintf (stderr, args); return; }

class Test_Class {
  public:
  Test_Class (void);
};

static Test_Class t;

static void
backtrace (void)
{
  char name[128], off[32];
  unw_word_t ip, offset;
  unw_cursor_t cursor;
  unw_context_t uc;
  int ret, count = 0;

  unw_getcontext (&uc);
  unw_init_local (&cursor, &uc);

  do
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      name[0] = '\0';
      off[0] = '\0';
      if (unw_get_proc_name (&cursor, name, sizeof (name), &offset) == 0
	  && off > 0)
	snprintf (off, sizeof (off), "+0x%lx", (long) offset);
      if (verbose)
	printf ("  [%lx] <%s%s>\n", (long) ip, name, off);
      if (++count > 32)
	panic ("FAILURE: didn't reach beginning of unwind-chain\n");
    }
  while ((ret = unw_step (&cursor)) > 0);

  if (ret < 0)
    panic ("FAILURE: unw_step() returned %d\n", ret);
}

static void
b (void)
{
  backtrace();
}

static void
a (void)
{
  if (verbose)
    printf ("backtrace() from atexit()-handler:\n");
  b();
  if (errors)
    abort ();	
}

Test_Class::Test_Class (void)
{
  if (verbose)
    printf ("backtrace() from constructor:\n");
  b();
}

int
main (int argc, char **argv)
{
  verbose = argc > 1;
  return atexit (a);
}
