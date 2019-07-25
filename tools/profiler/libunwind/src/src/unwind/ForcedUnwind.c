
























#include "unwind-internal.h"

PROTECTED _Unwind_Reason_Code
_Unwind_ForcedUnwind (struct _Unwind_Exception *exception_object,
		      _Unwind_Stop_Fn stop, void *stop_parameter)
{
  struct _Unwind_Context context;
  unw_context_t uc;

  


  if (!stop)
    return _URC_FATAL_PHASE2_ERROR;

  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FATAL_PHASE2_ERROR;

  exception_object->private_1 = (unsigned long) stop;
  exception_object->private_2 = (unsigned long) stop_parameter;

  return _Unwind_Phase2 (exception_object, &context);
}

_Unwind_Reason_Code __libunwind_Unwind_ForcedUnwind (struct _Unwind_Exception*,
						     _Unwind_Stop_Fn, void *)
     ALIAS (_Unwind_ForcedUnwind);
