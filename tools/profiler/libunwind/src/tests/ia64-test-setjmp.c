

























#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#ifdef HAVE_IA64INTRIN_H
# include <ia64intrin.h>
#endif

static sigjmp_buf env;
static int return_level;
static uintptr_t return_bsp;
static int verbose;

uintptr_t
get_bsp (void)
{
#ifdef __INTEL_COMPILER
  return __getReg (_IA64_REG_AR_BSP);
#else
  return (uintptr_t) __builtin_ia64_bsp ();
#endif
}

static void
sighandler (int signal, void *siginfo, void *sigcontext)
{
  ucontext_t *uc = sigcontext;
  int local = 0;

  if (verbose)
    printf ("got signal, stack at %p, saved bsp=0x%lx\n",
	    &local, uc->uc_mcontext.sc_ar_bsp);
  siglongjmp (env, 1);
}



static void doit (int n);
static volatile void (*doit_pointer) (int n) = doit;

static void
doit (int n)
{
  uintptr_t guard_page_addr, bsp = get_bsp ();
  void *ret;

  if (n == 0)
    {
      size_t page_size = getpagesize ();

      guard_page_addr = (bsp + page_size - 1) & -page_size;
      if (verbose)
	printf ("guard_page_addr = 0x%lx\n", (unsigned long) guard_page_addr);
      ret = mmap ((void *) guard_page_addr, page_size, PROT_NONE,
		  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (ret != (void *) guard_page_addr)
	{
	  if (ret == MAP_FAILED)
	    perror ("mmap");
	  else
	    fprintf (stderr, "mmap() returned %p, expected 0x%lx\n",
		     ret, guard_page_addr);
	  exit (EXIT_FAILURE);
	}
    }

  if (sigsetjmp (env, 1))
    {
      return_level = n;
      return_bsp = bsp;
    }
  else
    (*doit_pointer) (n + 1);
}

int
main (int argc, char **argv)
{
  struct sigaction sa;
  stack_t ss;

  if (argc > 1)
    verbose = 1;

  ss.ss_sp = malloc (2 * SIGSTKSZ);
  if (ss.ss_sp == NULL)
    {
      puts ("failed to allocate alternate stack");
      return EXIT_FAILURE;
    }
  ss.ss_flags = 0;
  ss.ss_size = 2 * SIGSTKSZ;
  if (sigaltstack (&ss, NULL) < 0)
    {
      printf ("sigaltstack failed: %s\n", strerror (errno));
      return EXIT_FAILURE;
    }

  sa.sa_handler = (void (*) (int)) sighandler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  if (sigaction (SIGSEGV, &sa, NULL) < 0)
    {
      printf ("sigaction failed: %s\n", strerror (errno));
      exit (1);
    }

  doit (0);

  if (verbose)
    {
      printf ("sigsetjmp returned at level %d bsp=0x%lx\n",
	      return_level, return_bsp);
      puts ("Test succeeded!");
    }
  return EXIT_SUCCESS;
}
