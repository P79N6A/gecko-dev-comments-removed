







































#ifndef mozilla_throw_msvc_h
#define mozilla_throw_msvc_h




#ifdef _EXCEPTION_
#  error "Unable to wrap _Throw(); CRT _Throw() already declared"
#endif

#define _Throw  mozilla_Throw
#include <exception>

#endif  
