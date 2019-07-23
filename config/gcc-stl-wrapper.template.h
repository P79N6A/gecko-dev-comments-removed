







































#ifndef mozilla_${HEADER}_h
#define mozilla_${HEADER}_h

#if __EXCEPTIONS
#  error "STL code can only be used with -fno-exceptions"
#endif




#if !defined(XPCOM_GLUE) && !defined(NS_NO_XPCOM) && !defined(MOZ_NO_MOZALLOC)
#  include <new>              
#  include <stdlib.h>         
#  include <string.h>
#  include "mozilla/mozalloc.h"
#else
#  error "STL code can only be used with infallible ::operator new()"
#endif

#if defined(DEBUG) && !defined(_GLIBCXX_DEBUG)







#endif

#pragma GCC visibility push(default)
#include_next <${HEADER}>
#pragma GCC visibility pop








#ifndef mozilla_functexcept_h
#  include "mozilla/functexcept.h"
#endif

#endif
