







































#include <stdio.h>

#if defined(XP_WIN) || (defined(XP_OS2)
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif

#define MOZALLOC_DONT_WRAP_RAISE_FUNCTIONS
#include "mozilla/throw_msvc.h"

__declspec(noreturn) static void abort_from_exception(const char* const which,
                                                      const char* const what);
static void
abort_from_exception(const char* const which,  const char* const what)
{
    fprintf(stderr, "fatal: STL threw %s: ", which);
    mozalloc_abort(what);
}

namespace std {





void
moz_Xinvalid_argument(const char* what)
{
    abort_from_exception("invalid_argument", what);
}

void
moz_Xlength_error(const char* what)
{
    abort_from_exception("length_error", what);
}

void
moz_Xout_of_range(const char* what)
{
    abort_from_exception("out_of_range", what);
}

void
moz_Xoverflow_error(const char* what)
{
    abort_from_exception("overflow_error", what);
}

void
moz_Xruntime_error(const char* what)
{
    abort_from_exception("runtime_error", what);
}

}  
