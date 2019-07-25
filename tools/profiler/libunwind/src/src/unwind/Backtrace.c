
























#include "unwind-internal.h"

PROTECTED _Unwind_Reason_Code
_Unwind_Backtrace (_Unwind_Trace_Fn trace, void *trace_parameter)
{
  struct _Unwind_Context context;
  unw_context_t uc;
  int ret;

  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FATAL_PHASE1_ERROR;

  

  while (1)
    {
      if ((ret = unw_step (&context.cursor)) <= 0)
	{
	  if (ret == 0)
	    return _URC_END_OF_STACK;
	  else
	    return _URC_FATAL_PHASE1_ERROR;
	}

      if ((*trace) (&context, trace_parameter) != _URC_NO_REASON)
	return _URC_FATAL_PHASE1_ERROR;
    }
}

_Unwind_Reason_Code __libunwind_Unwind_Backtrace (_Unwind_Trace_Fn, void *)
     ALIAS (_Unwind_Backtrace);
