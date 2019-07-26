







#ifndef mozilla_Attributes_h
#define mozilla_Attributes_h

#include "mozilla/Compiler.h"











#if defined(_MSC_VER)
#  define MOZ_ALWAYS_INLINE_EVEN_DEBUG     __forceinline
#elif defined(__GNUC__)
#  define MOZ_ALWAYS_INLINE_EVEN_DEBUG     __attribute__((always_inline)) inline
#else
#  define MOZ_ALWAYS_INLINE_EVEN_DEBUG     inline
#endif

#if !defined(DEBUG)
#  define MOZ_ALWAYS_INLINE     MOZ_ALWAYS_INLINE_EVEN_DEBUG
#elif defined(_MSC_VER) && !defined(__cplusplus)
#  define MOZ_ALWAYS_INLINE     __inline
#else
#  define MOZ_ALWAYS_INLINE     inline
#endif









#if defined(__clang__)
   




#  ifndef __has_extension
#    define __has_extension __has_feature /* compatibility, for older versions of clang */
#  endif
#  if __has_extension(cxx_constexpr)
#    define MOZ_HAVE_CXX11_CONSTEXPR
#  endif
#  if __has_extension(cxx_explicit_conversions)
#    define MOZ_HAVE_EXPLICIT_CONVERSION
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
#    if MOZ_GCC_VERSION_AT_LEAST(4, 5, 0)
#      define MOZ_HAVE_EXPLICIT_CONVERSION
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
#  if _MSC_VER >= 1800
#    define MOZ_HAVE_CXX11_DELETE
#  endif
#  if _MSC_VER >= 1700
#    define MOZ_HAVE_CXX11_FINAL         final
#  else
     
#    define MOZ_HAVE_CXX11_FINAL         sealed
#  endif
#  define MOZ_HAVE_CXX11_OVERRIDE
#  define MOZ_HAVE_NEVER_INLINE          __declspec(noinline)
#  define MOZ_HAVE_NORETURN              __declspec(noreturn)


#endif





#ifdef __clang_analyzer__
#  if __has_extension(attribute_analyzer_noreturn)
#    define MOZ_HAVE_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#  endif
#endif











#ifdef MOZ_HAVE_CXX11_CONSTEXPR
#  define MOZ_CONSTEXPR         constexpr
#  define MOZ_CONSTEXPR_VAR     constexpr
#else
#  define MOZ_CONSTEXPR
#  define MOZ_CONSTEXPR_VAR     const
#endif



















#ifdef MOZ_HAVE_EXPLICIT_CONVERSION
#  define MOZ_EXPLICIT_CONVERSION explicit
#else
#  define MOZ_EXPLICIT_CONVERSION
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
















#if defined(MOZ_HAVE_ANALYZER_NORETURN)
#  define MOZ_PRETEND_NORETURN_FOR_STATIC_ANALYSIS          MOZ_HAVE_ANALYZER_NORETURN
#else
#  define MOZ_PRETEND_NORETURN_FOR_STATIC_ANALYSIS
#endif








#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define MOZ_HAVE_ASAN_BLACKLIST
#  endif
#elif defined(__GNUC__)
#  if defined(__SANITIZE_ADDRESS__)
#    define MOZ_HAVE_ASAN_BLACKLIST
#  endif
#endif

#if defined(MOZ_HAVE_ASAN_BLACKLIST)
#  define MOZ_ASAN_BLACKLIST MOZ_NEVER_INLINE __attribute__((no_sanitize_address))
#else
#  define MOZ_ASAN_BLACKLIST
#endif







#if defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define MOZ_TSAN_BLACKLIST MOZ_NEVER_INLINE __attribute__((no_sanitize_thread))
#  else
#    define MOZ_TSAN_BLACKLIST
#  endif
#else
#  define MOZ_TSAN_BLACKLIST
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
#  define MOZ_MUST_OVERRIDE __attribute__((annotate("moz_must_override")))
#  define MOZ_STACK_CLASS __attribute__((annotate("moz_stack_class")))
#  define MOZ_NONHEAP_CLASS __attribute__((annotate("moz_nonheap_class")))





#  define MOZ_HEAP_ALLOCATOR \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wgcc-compat\"") \
    __attribute__((annotate("moz_heap_allocator"))) \
    _Pragma("clang diagnostic pop")
#else
#  define MOZ_MUST_OVERRIDE
#  define MOZ_STACK_CLASS
#  define MOZ_NONHEAP_CLASS
#  define MOZ_HEAP_ALLOCATOR
#endif 





#ifdef _MSC_VER
#  define MOZ_THIS_IN_INITIALIZER_LIST() \
     __pragma(warning(push)) \
     __pragma(warning(disable:4355)) \
     this \
     __pragma(warning(pop))
#else
#  define MOZ_THIS_IN_INITIALIZER_LIST() this
#endif

#endif 

#endif 
