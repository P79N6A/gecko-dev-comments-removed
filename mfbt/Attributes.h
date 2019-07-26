






#ifndef mozilla_Attributes_h_
#define mozilla_Attributes_h_

#include "mozilla/Compiler.h"







#if defined(__cplusplus)
#  define MOZ_INLINE            inline
#elif defined(_MSC_VER)
#  define MOZ_INLINE            __inline
#elif defined(__GNUC__)
#  define MOZ_INLINE            __inline__
#else
#  define MOZ_INLINE            inline
#endif








#if defined(_MSC_VER)
#  define MOZ_ALWAYS_INLINE     __forceinline
#elif defined(__GNUC__)
#  define MOZ_ALWAYS_INLINE     __attribute__((always_inline)) MOZ_INLINE
#else
#  define MOZ_ALWAYS_INLINE     MOZ_INLINE
#endif









#if defined(__clang__)
   




#  ifndef __has_extension
#    define __has_extension __has_feature /* compatibility, for older versions of clang */
#  endif
#  if __has_extension(cxx_constexpr)
#    define MOZ_HAVE_CXX11_CONSTEXPR
#  endif
#  if __has_extension(cxx_deleted_functions)
#    define MOZ_HAVE_CXX11_DELETE
#  endif
#  if __has_extension(cxx_override_control)
#    define MOZ_HAVE_CXX11_OVERRIDE
#    define MOZ_HAVE_CXX11_FINAL         final
#  endif
#  if __has_attribute(noinline)
#    define MOZ_HAVE_NEVER_INLINE        __attribute__((noinline))
#  endif
#  if __has_attribute(noreturn)
#    define MOZ_HAVE_NORETURN            __attribute__((noreturn))
#  endif
#elif defined(__GNUC__)
#  if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    if MOZ_GCC_VERSION_AT_LEAST(4, 7, 0)
#      define MOZ_HAVE_CXX11_OVERRIDE
#      define MOZ_HAVE_CXX11_FINAL       final
#    endif
#    if MOZ_GCC_VERSION_AT_LEAST(4, 6, 0)
#      define MOZ_HAVE_CXX11_CONSTEXPR
#    endif
#    define MOZ_HAVE_CXX11_DELETE
#  else
     
#    if MOZ_GCC_VERSION_AT_LEAST(4, 7, 0)
#      define MOZ_HAVE_CXX11_FINAL       __final
#    endif
#  endif
#  define MOZ_HAVE_NEVER_INLINE          __attribute__((noinline))
#  define MOZ_HAVE_NORETURN              __attribute__((noreturn))
#elif defined(_MSC_VER)
#  if _MSC_VER >= 1700
#    define MOZ_HAVE_CXX11_FINAL         final
#  else
     
#    define MOZ_HAVE_CXX11_FINAL         sealed
#  endif
#  define MOZ_HAVE_CXX11_OVERRIDE
#  define MOZ_HAVE_NEVER_INLINE          __declspec(noinline)
#  define MOZ_HAVE_NORETURN              __declspec(noreturn)
#endif






#ifdef MOZ_HAVE_CXX11_CONSTEXPR
#  define MOZ_CONSTEXPR         constexpr
#else
#  define MOZ_CONSTEXPR
#endif







#if defined(MOZ_HAVE_NEVER_INLINE)
#  define MOZ_NEVER_INLINE      MOZ_HAVE_NEVER_INLINE
#else
#  define MOZ_NEVER_INLINE
#endif















#if defined(MOZ_HAVE_NORETURN)
#  define MOZ_NORETURN          MOZ_HAVE_NORETURN
#else
#  define MOZ_NORETURN
#endif







#if defined(MOZ_ASAN)
#  define MOZ_ASAN_BLACKLIST MOZ_NEVER_INLINE __attribute__((no_address_safety_analysis))
# else
#  define MOZ_ASAN_BLACKLIST
#endif


#ifdef __cplusplus























#if defined(MOZ_HAVE_CXX11_DELETE)
#  define MOZ_DELETE            = delete
#else
#  define MOZ_DELETE
#endif




































#if defined(MOZ_HAVE_CXX11_OVERRIDE)
#  define MOZ_OVERRIDE          override
#else
#  define MOZ_OVERRIDE
#endif
































































#if defined(MOZ_HAVE_CXX11_FINAL)
#  define MOZ_FINAL             MOZ_HAVE_CXX11_FINAL
#else
#  define MOZ_FINAL
#endif














#if defined(__GNUC__) || defined(__clang__)
#  define MOZ_WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
#else
#  define MOZ_WARN_UNUSED_RESULT
#endif





















































#ifdef MOZ_CLANG_PLUGIN
# define MOZ_MUST_OVERRIDE __attribute__((annotate("moz_must_override")))
#else
# define MOZ_MUST_OVERRIDE
#endif 

#endif 

#endif  
