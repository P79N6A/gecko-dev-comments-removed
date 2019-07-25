

























#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libunwind.h>

#ifdef HAVE_IA64INTRIN_H
# include <ia64intrin.h>
#endif

#define panic(args...)				\
	{ ++nerrors; fprintf (stderr, args); }

int nerrors = 0;
int verbose = 0;
int depth = 13;
volatile int got_here = 0;

extern void b (int);

void
raise_exception (void)
{
  unw_cursor_t cursor;
  unw_context_t uc;
  int i;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    {
      panic ("unw_init_local() failed!\n");
      return;
    }

  
  for (i = 0; i < depth + 2; ++i)
    if (unw_step (&cursor) < 0)
      {
	panic ("unw_step() failed!\n");
	return;
      }
  unw_resume (&cursor);	
}

uintptr_t
get_bsp (void)
{
#if UNW_TARGET_IA64
# ifdef __INTEL_COMPILER
  return __getReg (_IA64_REG_AR_BSP);
# else
  return (uintptr_t) __builtin_ia64_bsp ();
# endif
#else
  return 0;
#endif
}

int
a (int n)
{
  long stack;
  int result = 99;

  if (verbose)
    printf ("a(n=%d): sp=%p bsp=0x%lx\n",
	    n, &stack, (unsigned long) get_bsp ());

  if (n > 0)
    a (n - 1) + 1;
  else
    b (16);

  if (verbose)
    {
      printf ("exception handler: here we go (sp=%p, bsp=0x%lx)...\n",
	      &stack, (unsigned long) get_bsp ());
      


      getpid ();
    }
  if (n == depth)
    {
      result = 0;
      got_here = 1;
    }
  return result;
}

void
b (int n)
{
  if ((n & 1) == 0)
    {
      if (verbose)
	printf ("b(n=%d) calling raise_exception()\n", n);
      raise_exception ();
    }
  panic ("FAILURE: b() returned from raise_exception()!!\n");
}

int
main (int argc, char **argv)
{
  int result;

  if (argc > 1)
    {
      ++verbose;
      depth = atol (argv[1]);
      if (depth < 1)
	{
	  fprintf (stderr, "Usage: %s depth\n"
		   "  depth must be >= 1\n", argv[0]);
	  exit (-1);
	}
    }

  result = a (depth);
  if (result != 0 || !got_here || nerrors > 0)
    {
      fprintf (stderr,
	       "FAILURE: test failed: result=%d got_here=%d nerrors=%d\n",
	       result, got_here, nerrors);
      exit (-1);
    }

  if (verbose)
    printf ("SUCCESS!\n");
  return 0;
}
