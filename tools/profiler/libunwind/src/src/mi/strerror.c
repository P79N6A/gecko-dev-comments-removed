
























#include "libunwind_i.h"



const char *
unw_strerror (int err_code)
{
  const char *cp;
  unw_error_t error = (unw_error_t)-err_code;
  switch (error)
    {
    case UNW_ESUCCESS:	   cp = "no error"; break;
    case UNW_EUNSPEC:	   cp = "unspecified (general) error"; break;
    case UNW_ENOMEM:	   cp = "out of memory"; break;
    case UNW_EBADREG:	   cp = "bad register number"; break;
    case UNW_EREADONLYREG: cp = "attempt to write read-only register"; break;
    case UNW_ESTOPUNWIND:  cp = "stop unwinding"; break;
    case UNW_EINVALIDIP:   cp = "invalid IP"; break;
    case UNW_EBADFRAME:	   cp = "bad frame"; break;
    case UNW_EINVAL:	   cp = "unsupported operation or bad value"; break;
    case UNW_EBADVERSION:  cp = "unwind info has unsupported version"; break;
    case UNW_ENOINFO:	   cp = "no unwind info found"; break;
    default:		   cp = "invalid error code";
    }
  return cp;
}
