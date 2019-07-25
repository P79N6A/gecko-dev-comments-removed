
























#define UNW_LOCAL_ONLY

#undef _FORTIFY_SOURCE
#include <assert.h>
#include <libunwind.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

#include "jmpbuf.h"
#include "setjmp_i.h"

#if defined(__GLIBC__)
#if __GLIBC_PREREQ(2, 4)










#define _longjmp __nonworking__longjmp
#define longjmp __nonworking_longjmp
static void _longjmp (jmp_buf env, int val);
static void longjmp (jmp_buf env, int val);
#endif
#endif 

void
_longjmp (jmp_buf env, int val)
{
  extern int _UI_longjmp_cont;
  unw_context_t uc;
  unw_cursor_t c;
  unw_word_t sp;
  unw_word_t *wp = (unw_word_t *) env;

  if (unw_getcontext (&uc) < 0 || unw_init_local (&c, &uc) < 0)
    abort ();

  do
    {
      if (unw_get_reg (&c, UNW_REG_SP, &sp) < 0)
	abort ();
#ifdef __FreeBSD__
      if (sp != wp[JB_SP] + sizeof(unw_word_t))
#else
      if (sp != wp[JB_SP])
#endif
	continue;

      if (!bsp_match (&c, wp))
	continue;

      

      assert (UNW_NUM_EH_REGS >= 2);

      if (unw_set_reg (&c, UNW_REG_EH + 0, wp[JB_RP]) < 0
	  || unw_set_reg (&c, UNW_REG_EH + 1, val) < 0
	  || unw_set_reg (&c, UNW_REG_IP,
			  (unw_word_t) (uintptr_t) &_UI_longjmp_cont))
	abort ();

      unw_resume (&c);

      abort ();
    }
  while (unw_step (&c) > 0);

  abort ();
}

#ifdef __GNUC__
#define STRINGIFY1(x) #x
#define STRINGIFY(x) STRINGIFY1(x)
void longjmp (jmp_buf env, int val) 
  __attribute__ ((alias (STRINGIFY(_longjmp))));
#else

void
longjmp (jmp_buf env, int val)
{
  _longjmp (env, val);
}

#endif 
