
























#include "unwind-internal.h"

PROTECTED void
_Unwind_DeleteException (struct _Unwind_Exception *exception_object)
{
  _Unwind_Exception_Cleanup_Fn cleanup = exception_object->exception_cleanup;

  if (cleanup)
    (*cleanup) (_URC_FOREIGN_EXCEPTION_CAUGHT, exception_object);
}

void __libunwind_Unwind_DeleteException (struct _Unwind_Exception *)
     ALIAS (_Unwind_DeleteException);
