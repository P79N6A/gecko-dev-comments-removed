
























#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>

pid_t self;
int global[64];

int
func (int arg)
{
  int sum = 0, i, max, arr[1024];

  if (arg == 0)
    {
      sum = global[2];
      sum += sum + sum * getppid ();
      return sum;
    }
  else
    {
      max = arg;
      if (max >= 64)
	max = 64;

      for (i = 0; i < max; ++i)
	arr[i] = func (arg - 1);

      for (i = 0; i < max; ++i)
	if (arr[i] > 16)
	  sum += arr[i];
	else
	  sum -= arr[i];
    }
  return sum;
}

int
bar (int v)
{
  extern long f (long);
  int arr[1] = { v };
  uintptr_t r;

  



  r = (uintptr_t) malloc(f (arr[0])
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + f (v))
       ))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))
       )))))))))))))))))))))))))))))))))))))))))))))))))))))));
  if (r < 2)
    v = r;

  kill (self, SIGUSR1);	
  v = func (v);
  kill (self, SIGUSR2);	
  return v;
}

int
main (int argc, char **argv)
{
  int val = argc;

  signal (SIGUSR1, SIG_IGN);
  signal (SIGUSR2, SIG_IGN);

  self = getpid ();

  printf ("sum = %d\n", bar (val));
  return 0;
}
