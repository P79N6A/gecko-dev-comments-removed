
























#include "unwind-internal.h"

PROTECTED void
_Unwind_Resume (struct _Unwind_Exception *exception_object)
{
  struct _Unwind_Context context;
  unw_context_t uc;

  if (_Unwind_InitContext (&context, &uc) < 0)
    abort ();

  _Unwind_Phase2 (exception_object, &context);
  abort ();
}

void __libunwind_Unwind_Resume (struct _Unwind_Exception *)
     ALIAS (_Unwind_Resume);
