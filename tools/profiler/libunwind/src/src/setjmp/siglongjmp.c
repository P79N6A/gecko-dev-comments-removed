
























#define UNW_LOCAL_ONLY

#include <setjmp.h>

#include "libunwind_i.h"
#include "jmpbuf.h"
#include "setjmp_i.h"

#if !defined(_NSIG) && defined(_SIG_MAXSIG)
# define _NSIG (_SIG_MAXSIG - 1)
#endif

#if defined(__GLIBC__)
#if __GLIBC_PREREQ(2, 4)











#define siglongjmp __nonworking_siglongjmp
static void siglongjmp (sigjmp_buf env, int val);
#endif
#endif 

void
siglongjmp (sigjmp_buf env, int val)
{
  unw_word_t *wp = (unw_word_t *) env;
  extern int _UI_siglongjmp_cont;
  extern int _UI_longjmp_cont;
  unw_context_t uc;
  unw_cursor_t c;
  unw_word_t sp;
  int *cont;

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

      

      
      cont = &_UI_longjmp_cont;

      


      if (!resume_restores_sigmask (&c, wp) && wp[JB_MASK_SAVED])
	{
	  
#if defined(__linux__)
	  if (UNW_NUM_EH_REGS < 4 || _NSIG > 16 * sizeof (unw_word_t))
	    


	    abort ();
	  else
	    if (unw_set_reg (&c, UNW_REG_EH + 2, wp[JB_MASK]) < 0
		|| (_NSIG > 8 * sizeof (unw_word_t)
		    && unw_set_reg (&c, UNW_REG_EH + 3, wp[JB_MASK + 1]) < 0))
	      abort ();
#elif defined(__FreeBSD__)
	  if (unw_set_reg (&c, UNW_REG_EH + 2, &wp[JB_MASK]) < 0)
	      abort();
#endif
	  cont = &_UI_siglongjmp_cont;
	}

      if (unw_set_reg (&c, UNW_REG_EH + 0, wp[JB_RP]) < 0
	  || unw_set_reg (&c, UNW_REG_EH + 1, val) < 0
	  || unw_set_reg (&c, UNW_REG_IP, (unw_word_t) (uintptr_t) cont))
	abort ();

      unw_resume (&c);

      abort ();
    }
  while (unw_step (&c) > 0);

  abort ();
}
