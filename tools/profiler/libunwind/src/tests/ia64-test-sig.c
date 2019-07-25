

























#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <libunwind-ia64.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

int verbose;

static void
sighandler (int signal)
{
  unw_cursor_t cursor, cursor2;
  unw_word_t ip;
  unw_context_t uc;

  if (verbose)
    printf ("caught signal %d\n", signal);

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init() failed!\n");

  
  if (unw_step (&cursor) < 0)
    panic ("unw_step() failed!\n");

  cursor2 = cursor;
  while (!unw_is_signal_frame (&cursor2))
    if (unw_step (&cursor2) < 0)
      panic ("failed to find signal frame!\n");

  if (unw_step (&cursor2) < 0)
    panic ("unw_step() failed!\n");

  if (unw_get_reg (&cursor2, UNW_REG_IP, &ip) < 0)
    panic ("failed to get IP!\n");

  
  ++ip;
  if ((ip & 0x3) == 0x3)
    ip += 13;

  if (unw_set_reg (&cursor2, UNW_REG_IP, ip) < 0)
    panic ("failed to set IP!\n");

  unw_resume (&cursor);	

  panic ("unexpected return from unw_resume()!\n");
}

static void
doit (volatile char *p)
{
  int ch;

  ch = *p;	

  if (verbose)
    printf ("doit: finishing execution!\n");
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    verbose = 1;

  signal (SIGSEGV, sighandler);
  doit (0);
  if (verbose)
    printf ("SUCCESS\n");
  return 0;
}
