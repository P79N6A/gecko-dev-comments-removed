






#ifndef mozilla_${HEADER}_h
#define mozilla_${HEADER}_h

#if _HAS_EXCEPTIONS
#  error "STL code can only be used with -fno-exceptions"
#endif



#ifndef mozilla_Throw_h
#  include "mozilla/throw_msvc.h"
#endif





#include <${NEW_HEADER_PATH}>




#if !defined(XPCOM_GLUE) && !defined(NS_NO_XPCOM) && !defined(MOZ_NO_MOZALLOC)
#  include "mozilla/mozalloc.h"
#else
#  error "STL code can only be used with infallible ::operator new()"
#endif

#ifdef DEBUG













#else










#endif





#pragma warning( push )
#pragma warning( disable : 4275 4530 )

#include <${HEADER_PATH}>

#pragma warning( pop )

#endif
