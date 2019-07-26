









#ifndef mozilla_NullPtr_h_
#define mozilla_NullPtr_h_

#if defined(__clang__)
#  ifndef __has_extension
#    define __has_extension __has_feature
#  endif
#  if __has_extension(cxx_nullptr)
#    define MOZ_HAVE_CXX11_NULLPTR
#  endif
#elif defined(__GNUC__)
#  if defined(_GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    if (__GNUC__ * 1000 + __GNUC_MINOR__) >= 4006
#      define MOZ_HAVE_CXX11_NULLPTR
#    endif
#  endif
#elif _MSC_VER >= 1600
# define MOZ_HAVE_CXX11_NULLPTR
#endif






#ifndef MOZ_HAVE_CXX11_NULLPTR
#  if defined(__GNUC__)
#    define nullptr __null
#  elif defined(_WIN64)
#    define nullptr 0LL
#  else
#    define nullptr 0L
#  endif
#endif

#endif  
