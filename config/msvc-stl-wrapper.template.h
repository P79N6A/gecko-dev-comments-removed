







































#ifndef mozilla_${HEADER}_h
#define mozilla_${HEADER}_h

#if _HAS_EXCEPTIONS
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

#ifdef DEBUG













#else










#endif



#pragma warning( push )
#pragma warning( disable : 4530 )

#include <${HEADER_PATH}>

#pragma warning( pop )

#endif
