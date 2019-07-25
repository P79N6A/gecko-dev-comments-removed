
























#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libunwind.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_IA64INTRIN_H
# include <ia64intrin.h>
#endif

#define panic(args...)						\
	do { fprintf (stderr, args); ++nerrors; } while (0)

int verbose;
int nerrors;
int got_usr1, got_usr2;
char *sigusr1_sp;

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

void
handler (int sig)
{
  unw_word_t ip;
  sigset_t mask, oldmask;
  unw_context_t uc;
  unw_cursor_t c;
  char foo;
  int ret;

#if UNW_TARGET_IA64
  if (verbose)
    printf ("bsp = %llx\n", (unsigned long long) get_bsp ());
#endif

  if (verbose)
    printf ("got signal %d\n", sig);

  if (sig == SIGUSR1)
    {
      ++got_usr1;
      sigusr1_sp = &foo;

      sigemptyset (&mask);
      sigaddset (&mask, SIGUSR2);
      sigprocmask (SIG_BLOCK, &mask, &oldmask);
      kill (getpid (), SIGUSR2);	

      signal (SIGUSR1, SIG_IGN);
      signal (SIGUSR2, handler);

      if ((ret = unw_getcontext (&uc)) < 0)
	panic ("unw_getcontext() failed: ret=%d\n", ret);
#if UNW_TARGET_X86_64
      
      uc.uc_sigmask = oldmask; 
#endif
      if ((ret = unw_init_local (&c, &uc)) < 0)
	panic ("unw_init_local() failed: ret=%d\n", ret);

      if ((ret = unw_step (&c)) < 0)		
	panic ("unw_step(1) failed: ret=%d\n", ret);

      if ((ret = unw_step (&c)) < 0)		
	panic ("unw_step(2) failed: ret=%d\n", ret);

      if ((ret = unw_get_reg (&c, UNW_REG_IP, &ip)) < 0)
	panic ("unw_get_reg(IP) failed: ret=%d\n", ret);
      if (verbose)
	printf ("resuming at 0x%lx, with SIGUSR2 pending\n",
		(unsigned long) ip);
      unw_resume (&c);
    }
  else if (sig == SIGUSR2)
    {
      ++got_usr2;
      if (got_usr1)
	{
	  if (verbose)
	    printf ("OK: stack still at %p\n", &foo);
	}
      signal (SIGUSR2, SIG_IGN);
    }
  else
    panic ("Got unexpected signal %d\n", sig);
}

int
main (int argc, char **argv)
{
  float d = 1.0;
  int n = 0;

  if (argc > 1)
    verbose = 1;

  signal (SIGUSR1, handler);

  


  while (d > 0.0)
    {
      d /= 2.0;
      ++n;
    }
  if (n > 9999)
    return -1;	

  if (verbose)
    printf ("sending SIGUSR1\n");
  kill (getpid (), SIGUSR1);

  if (!got_usr2)
    panic ("failed to get SIGUSR2\n");

  if (nerrors)
    {
      fprintf (stderr, "FAILURE: detected %d errors\n", nerrors);
      exit (-1);
    }

  if (verbose)
    printf ("SUCCESS\n");
  return 0;
}
