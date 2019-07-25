






















#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

int
main (int argc, char **argv, char **envp)
{
  char *program, **child_argv;
  struct timeval start, stop;
  double secs;
  int status, i;
  long count;
  pid_t pid;

  count = atol (argv[1]);
  program = argv[2];

  child_argv = alloca ((argc - 1) * sizeof (char *));
  for (i = 0; i < argc - 2; ++i)
    child_argv[i] = argv[2 + i];
  child_argv[i] = NULL;

  gettimeofday (&start, NULL);
  for (i = 0; i < count; ++i)
    {
      pid = fork ();
      if (pid == 0)
        {
          execve (program, child_argv, envp);
          _exit (-1);
        }
      else
        {
          waitpid (pid, &status, 0);
          if (!WIFEXITED (status) || WEXITSTATUS (status) != 0)
            {
              fprintf (stderr, "%s: child failed\n", argv[0]);
              exit (-1);
            }
        }
    }
  gettimeofday (&stop, NULL);

  secs = ((stop.tv_sec + 1e-6 * stop.tv_usec)
	  - (start.tv_sec + 1e-6 * start.tv_usec));
  printf ("%lu nsec/execution\n",
	  (unsigned long) (1e9 * secs / (double) count));
  return 0;
}
