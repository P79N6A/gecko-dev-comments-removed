










#ifndef mozilla_NullPtr_h
#define mozilla_NullPtr_h

#if defined(__clang__)
#  if !__has_extension(cxx_nullptr)
#    error "clang version natively supporting nullptr is required."
#  endif
#  define MOZ_HAVE_CXX11_NULLPTR
#elif defined(__GNUC__)
#  if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    include "mozilla/Compiler.h"
#    if MOZ_GCC_VERSION_AT_LEAST(4, 6, 0)
#      define MOZ_HAVE_CXX11_NULLPTR
#    endif
#  endif
#elif defined(_MSC_VER)
   
#  define MOZ_HAVE_CXX11_NULLPTR
#endif

namespace mozilla {




































template<typename T>
struct IsNullPointer { static const bool value = false; };

} 





















#ifdef MOZ_HAVE_CXX11_NULLPTR

namespace mozilla {
typedef decltype(nullptr) NullptrT;
template<>
struct IsNullPointer<decltype(nullptr)> { static const bool value = true; };
}
#  undef MOZ_HAVE_CXX11_NULLPTR
#elif MOZ_IS_GCC
#  define nullptr __null



namespace mozilla { typedef void* NullptrT; }
#else
#  error "No compiler support for nullptr or its emulation."
#endif

#endif 
