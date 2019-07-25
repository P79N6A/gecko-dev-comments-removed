
























#ifndef unwind_internal_h
#define unwind_internal_h

#define UNW_LOCAL_ONLY

#include <unwind.h>
#include <stdlib.h>
#include <libunwind.h>

#include "libunwind_i.h"


#define _U_VERSION	1

typedef _Unwind_Reason_Code (*_Unwind_Personality_Fn)
	(int, _Unwind_Action, uint64_t, struct _Unwind_Exception *,
	 struct _Unwind_Context *);

struct _Unwind_Context {
  unw_cursor_t cursor;
  int end_of_stack;	
};




#define _Unwind_InitContext(context, uc)				     \
  ((context)->end_of_stack = 0,						     \
   ((unw_getcontext (uc) < 0 || unw_init_local (&(context)->cursor, uc) < 0) \
    ? -1 : 0))

static _Unwind_Reason_Code ALWAYS_INLINE
_Unwind_Phase2 (struct _Unwind_Exception *exception_object,
		struct _Unwind_Context *context)
{
  _Unwind_Stop_Fn stop = (_Unwind_Stop_Fn) exception_object->private_1;
  uint64_t exception_class = exception_object->exception_class;
  void *stop_parameter = (void *) exception_object->private_2;
  _Unwind_Personality_Fn personality;
  _Unwind_Reason_Code reason;
  _Unwind_Action actions;
  unw_proc_info_t pi;
  unw_word_t ip;
  int ret;

  actions = _UA_CLEANUP_PHASE;
  if (stop)
    actions |= _UA_FORCE_UNWIND;

  while (1)
    {
      ret = unw_step (&context->cursor);
      if (ret <= 0)
	{
	  if (ret == 0)
	    {
	      actions |= _UA_END_OF_STACK;
	      context->end_of_stack = 1;
	    }
	  else
	    return _URC_FATAL_PHASE2_ERROR;
	}

      if (stop)
	{
	  reason = (*stop) (_U_VERSION, actions, exception_class,
			    exception_object, context, stop_parameter);
	  if (reason != _URC_NO_REASON)
	    




	    return _URC_FATAL_PHASE2_ERROR;
	}

      if (context->end_of_stack
	  || unw_get_proc_info (&context->cursor, &pi) < 0)
	return _URC_FATAL_PHASE2_ERROR;

      personality = (_Unwind_Personality_Fn) (uintptr_t) pi.handler;
      if (personality)
	{
	  if (!stop)
	    {
	      if (unw_get_reg (&context->cursor, UNW_REG_IP, &ip) < 0)
		return _URC_FATAL_PHASE2_ERROR;

	      if ((unsigned long) stop_parameter == ip)
		actions |= _UA_HANDLER_FRAME;
	    }

	  reason = (*personality) (_U_VERSION, actions, exception_class,
				   exception_object, context);
	  if (reason != _URC_CONTINUE_UNWIND)
	    {
	      if (reason == _URC_INSTALL_CONTEXT)
		{
		  
		  unw_resume (&context->cursor);
		  abort ();
		}
	      else
		return _URC_FATAL_PHASE2_ERROR;
	    }
	  if (actions & _UA_HANDLER_FRAME)
	    

	    abort ();
	}
    }
  return _URC_FATAL_PHASE2_ERROR;	
}

#endif 
