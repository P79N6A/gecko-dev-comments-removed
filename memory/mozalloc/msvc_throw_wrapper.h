







































#ifndef mozilla_msvc_throw_wrapper_h
#define mozilla_msvc_throw_wrapper_h


#  ifdef _EXCEPTION_
#    error "Unable to wrap _Throw(); CRT _Throw() already declared"
#  endif
#  define _Throw  mozilla_Throw
#  include <exception>

#endif  
