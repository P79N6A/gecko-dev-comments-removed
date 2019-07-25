
























#include "unwind-internal.h"

PROTECTED _Unwind_Reason_Code
_Unwind_RaiseException (struct _Unwind_Exception *exception_object)
{
  uint64_t exception_class = exception_object->exception_class;
  _Unwind_Personality_Fn personality;
  struct _Unwind_Context context;
  _Unwind_Reason_Code reason;
  unw_proc_info_t pi;
  unw_context_t uc;
  unw_word_t ip;
  int ret;

  Debug (1, "(exception_object=%p)\n", exception_object);

  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FATAL_PHASE1_ERROR;

  

  while (1)
    {
      if ((ret = unw_step (&context.cursor)) <= 0)
	{
	  if (ret == 0)
	    {
	      Debug (1, "no handler found\n");
	      return _URC_END_OF_STACK;
	    }
	  else
	    return _URC_FATAL_PHASE1_ERROR;
	}

      if (unw_get_proc_info (&context.cursor, &pi) < 0)
	return _URC_FATAL_PHASE1_ERROR;

      personality = (_Unwind_Personality_Fn) (uintptr_t) pi.handler;
      if (personality)
	{
	  reason = (*personality) (_U_VERSION, _UA_SEARCH_PHASE,
				   exception_class, exception_object,
				   &context);
	  if (reason != _URC_CONTINUE_UNWIND)
	    {
	      if (reason == _URC_HANDLER_FOUND)
		break;
	      else
		{
		  Debug (1, "personality returned %d\n", reason);
		  return _URC_FATAL_PHASE1_ERROR;
		}
	    }
	}
    }

  




  if (unw_get_reg (&context.cursor, UNW_REG_IP, &ip) < 0)
    return _URC_FATAL_PHASE1_ERROR;
  exception_object->private_1 = 0;	
  exception_object->private_2 = ip;	

  Debug (1, "found handler for IP=%lx; entering cleanup phase\n", (long) ip);

  
  if (unw_init_local (&context.cursor, &uc) < 0)
    return _URC_FATAL_PHASE1_ERROR;

  return _Unwind_Phase2 (exception_object, &context);
}

_Unwind_Reason_Code
__libunwind_Unwind_RaiseException (struct _Unwind_Exception *)
     ALIAS (_Unwind_RaiseException);
