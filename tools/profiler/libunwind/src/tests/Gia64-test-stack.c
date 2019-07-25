





























#include <libunwind.h>
#include <stdio.h>
#include <stdlib.h>

#include "ia64-test-stack.h"

#define panic(args...)				\
	{ printf (args); ++nerrors; }







#define MAX_PHYS_STACKED	2048
#define RECURSION_DEPTH		((MAX_PHYS_STACKED + 94) / 95)

extern void touch_all (unsigned long recursion_depth);
extern void flushrs (void);

int nerrors;
int verbose;

void
do_unwind_tests (void)
{
  unw_word_t ip, sp, bsp, v0, v1, v2, v3, n0, n1, n2, n3, cfm, sof, sol, r32;
  int ret, reg, i, l;
  unw_context_t uc;
  unw_cursor_t c;

  if (verbose)
    printf ("do_unwind_tests: here we go!\n");

  
  unw_getcontext (&uc);
  unw_init_local (&c, &uc);
  i = 0;
  do
    {
      if (verbose)
	{
	  if ((ret = unw_get_reg (&c, UNW_IA64_IP, &ip)) < 0
	      || (ret = unw_get_reg (&c, UNW_IA64_SP, &sp)) < 0
	      || (ret = unw_get_reg (&c, UNW_IA64_BSP, &bsp)) < 0)
	    break;
	  printf ("ip=0x%16lx sp=0x%16lx bsp=0x%16lx\n", ip, sp, bsp);

	  for (reg = 32; reg < 128; reg += 4)
	    {
	      v0 = v1 = v2 = v3 = 0;
	      n0 = n1 = n2 = n3 = 0;
	      ((ret = unw_get_reg (&c, UNW_IA64_GR + reg, &v0)) < 0
	       || (ret = unw_get_reg (&c, UNW_IA64_NAT + reg, &n0)) < 0
	       || (ret = unw_get_reg (&c, UNW_IA64_GR  + reg + 1, &v1)) < 0
	       || (ret = unw_get_reg (&c, UNW_IA64_NAT + reg + 1, &n1)) < 0
	       || (ret = unw_get_reg (&c, UNW_IA64_GR  + reg + 2, &v2)) < 0
	       || (ret = unw_get_reg (&c, UNW_IA64_NAT + reg + 2, &n2)) < 0
	       || (ret = unw_get_reg (&c, UNW_IA64_GR  + reg + 3, &v3)) < 0
	   || (ret = unw_get_reg (&c, UNW_IA64_NAT + reg + 3, &n3)) < 0);
	      if (reg < 100)
		printf ("  r%d", reg);
	      else
		printf (" r%d", reg);
	      printf (" %c%016lx %c%016lx %c%016lx %c%016lx\n",
		      n0 ? '*' : ' ', v0, n1 ? '*' : ' ', v1,
		      n2 ? '*' : ' ', v2, n3 ? '*' : ' ', v3);
	      if (ret < 0)
		break;
	    }
	}

      if (i >= 1 && i <= NSTACKS)
	{
	  if ((ret = unw_get_reg (&c, UNW_IA64_CFM, &cfm)) < 0)
	    break;
	  sof = cfm & 0x7f;
	  if (sof != (unw_word_t) (i & 1))
	      panic ("\texpected sof=%d, found sof=%lu\n", i - 1, sof);
	  if (sof == 1)
	    {
	      if ((ret = unw_get_reg (&c, UNW_IA64_GR + 32, &r32)) < 0)
		break;
	      if (r32 != (unw_word_t) (i - 1))
		panic ("\texpected r32=%d, found r32=%lu\n", i - 1, r32);
	    }
	}
      else if (i > NSTACKS && i <= NSTACKS + RECURSION_DEPTH)
	{
	  if ((ret = unw_get_reg (&c, UNW_IA64_CFM, &cfm)) < 0)
	    break;
	  sof = cfm & 0x7f;
	  sol = (cfm >> 7) & 0x7f;
	  if (sof != 96)
	      panic ("\texpected sof=96, found sof=%lu\n", sof);
	  if (sol != 95)
	      panic ("\texpected sol=95, found sol=%lu\n", sol);

	  for (l = 2; l <= 93; ++l)
	    {
	      if ((ret = unw_get_reg (&c, UNW_IA64_GR + 33 + l, &v0)) < 0
		  || (ret = unw_get_reg (&c, UNW_IA64_NAT + 33 + l, &n0)) < 0)
		break;
	      switch (l)
		{
		case 2: case 31: case 73: case 93:
		  if (!n0)
		    panic ("\texpected loc%d to be a NaT!\n", l);
		  break;

		default:
		  if (n0)
		    panic ("\tloc%d is unexpectedly a NaT!\n", l);
		  v1 = ((unw_word_t) (i - NSTACKS) << 32) + l;
		  if (v0 != v1)
		    panic ("\tloc%d expected to be %lx, found to be %lx\n",
			   l, v1, v0);
		}
	    }
	}
      ++i;
    }
  while ((ret = unw_step (&c)) > 0);

  if (ret < 0)
    panic ("libunwind returned %d\n", ret);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    ++verbose;

  touch_all (RECURSION_DEPTH);
  if (nerrors)
    {
      printf ("FAILURE: detected %d errors\n", nerrors);
      exit (-1);
    }
  if (verbose)
    printf ("SUCCESS\n");
  return 0;
}
