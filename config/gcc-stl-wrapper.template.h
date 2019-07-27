






#ifndef mozilla_${HEADER}_h
#define mozilla_${HEADER}_h



#if defined(__EXCEPTIONS) && __EXCEPTIONS && !(__OBJC__ && __GNUC__ && XP_IOS)
#  error "STL code can only be used with -fno-exceptions"
#endif


#pragma GCC system_header

#ifdef _WIN32

#define NOMINMAX 1
#endif









#include_next <new>




#if !defined(XPCOM_GLUE) && !defined(NS_NO_XPCOM) && !defined(MOZ_NO_MOZALLOC)
#  include "mozilla/mozalloc.h"
#else
#  error "STL code can only be used with infallible ::operator new()"
#endif

#if defined(DEBUG) && !defined(_GLIBCXX_DEBUG)







#endif

#pragma GCC visibility push(default)
#include_next <${HEADER}>
#pragma GCC visibility pop








#ifndef mozilla_throw_gcc_h
#  include "mozilla/throw_gcc.h"
#endif

#endif
