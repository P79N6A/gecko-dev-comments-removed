






























#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
# define MAP_ANONYMOUS MAP_ANON
#endif

int
main (int argc, char **argv)
{
  long n = 0;

  signal (SIGUSR1, SIG_IGN);
  signal (SIGUSR2, SIG_IGN);

  printf ("Starting mmap test...\n");
  for (n = 0; n < 30000; ++n)
    {
      if (mmap (0, 1, (n & 1) ? PROT_READ : PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
		-1, 0) == MAP_FAILED)
	{
	  printf ("Failed after %ld successful maps\n", n - 1);
	  exit (0);
	}
    }

  alarm (80);	

  printf ("Turning on single-stepping...\n");
  kill (getpid (), SIGUSR1);	
  printf ("Va bene?\n");
  kill (getpid (), SIGUSR2);	
  printf ("Turned single-stepping off...\n");
  return 0;
}
