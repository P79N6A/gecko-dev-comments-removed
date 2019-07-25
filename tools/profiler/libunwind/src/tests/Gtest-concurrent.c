
























#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libunwind.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NTHREADS	128

#define panic(args...)						\
	do { fprintf (stderr, args); ++nerrors; } while (0)

int verbose;
int nerrors;
int got_usr1, got_usr2;
char *sigusr1_sp;

void
handler (int sig)
{
  unw_word_t ip;
  unw_context_t uc;
  unw_cursor_t c;
  int ret;

  unw_getcontext (&uc);
  unw_init_local (&c, &uc);
  do
    {
      unw_get_reg (&c, UNW_REG_IP, &ip);
      if (verbose)
	printf ("%lx: IP=%lx\n", (long) pthread_self (), (unsigned long) ip);
    }
  while ((ret = unw_step (&c)) > 0);

  if (ret < 0)
    panic ("unw_step() returned %d\n", ret);
}

void *
worker (void *arg)
{
  signal (SIGUSR1, handler);

  if (verbose)
    printf ("sending SIGUSR1\n");
  pthread_kill (pthread_self (), SIGUSR1);
  return NULL;
}

static void
doit (void)
{
  pthread_t th[NTHREADS];
  pthread_attr_t attr;
  int i;

  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, PTHREAD_STACK_MIN + 64*1024);

  for (i = 0; i < NTHREADS; ++i)
    if (pthread_create (th + i, &attr, worker, NULL))
      {
	fprintf (stderr, "FAILURE: Failed to create %u threads "
		 "(after %u threads)\n",
		 NTHREADS, i);
	exit (-1);
      }

  for (i = 0; i < NTHREADS; ++i)
    pthread_join (th[i], NULL);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    verbose = 1;

  if (verbose)
    printf ("Caching: none\n");
  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_NONE);
  doit ();

  if (verbose)
    printf ("Caching: global\n");
  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_GLOBAL);
  doit ();

  if (verbose)
    printf ("Caching: per-thread\n");
  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_PER_THREAD);
  doit ();

  if (nerrors)
    {
      fprintf (stderr, "FAILURE: detected %d errors\n", nerrors);
      exit (-1);
    }

  if (verbose)
    printf ("SUCCESS\n");
  return 0;
}
