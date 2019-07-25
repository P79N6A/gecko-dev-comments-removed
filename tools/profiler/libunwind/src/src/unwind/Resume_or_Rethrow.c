
























#include "unwind-internal.h"

PROTECTED _Unwind_Reason_Code
_Unwind_Resume_or_Rethrow (struct _Unwind_Exception *exception_object)
{
  struct _Unwind_Context context;
  unw_context_t uc;

  if (exception_object->private_1)
    {
      if (_Unwind_InitContext (&context, &uc) < 0)
	return _URC_FATAL_PHASE2_ERROR;

      return _Unwind_Phase2 (exception_object, &context);
    }
  else
    return _Unwind_RaiseException (exception_object);
}

_Unwind_Reason_Code
__libunwind_Unwind_Resume_or_Rethrow (struct _Unwind_Exception *)
     ALIAS (_Unwind_Resume_or_Rethrow);
