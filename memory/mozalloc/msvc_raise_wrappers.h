







































#ifndef mozilla_msvc_raise_wrappers_h
#define mozilla_msvc_raise_wrappers_h

#ifdef _XSTDDEF_
#  error "Unable to wrap _RAISE(); CRT _RAISE() already defined"
#endif
#ifdef _XUTILITY_
#  error "Unabled to wrap _X[exception]"(); CRT versions already declared"
#endif

#include "mozilla/mozalloc_abort.h"

namespace std {





MOZALLOC_EXPORT __declspec(noreturn) void moz_Xinvalid_argument(const char*);
MOZALLOC_EXPORT __declspec(noreturn) void moz_Xlength_error(const char*);
MOZALLOC_EXPORT __declspec(noreturn) void moz_Xout_of_range(const char*);
MOZALLOC_EXPORT __declspec(noreturn) void moz_Xoverflow_error(const char*);
MOZALLOC_EXPORT __declspec(noreturn) void moz_Xruntime_error(const char*);

} 

#ifndef MOZALLOC_DONT_WRAP_RAISE_FUNCTIONS

#  define _Xinvalid_argument  moz_Xinvalid_argument
#  define _Xlength_error      moz_Xlength_error
#  define _Xout_of_range      moz_Xout_of_range
#  define _Xoverflow_error    moz_Xoverflow_error
#  define _Xruntime_error     moz_Xruntime_error

#  include <xstddef>
#  include <xutility>

#  undef _RAISE
#  define _RAISE(x) mozalloc_abort((x).what())

#endif  

#endif  
