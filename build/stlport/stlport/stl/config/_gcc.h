



#define _STLP_COMPILER "gcc"

#define _STLP_HAS_INCLUDE_NEXT 1

#if (__GNUC__ < 2) || ((__GNUC__ < 3) && ((__GNUC_MINOR__ < 95) || (__GNUC_MINOR__ == 96)))

#  error GNU compilers before 2.95 are not supported anymore.
#endif


#if defined (__linux__)
#  ifndef _STLP_USE_GLIBC
#    define _STLP_USE_GLIBC 1
#  endif
#  if defined (__UCLIBC__) && !defined (_STLP_USE_UCLIBC)
#    define _STLP_USE_UCLIBC 1
#  endif
#endif

#if defined (__CYGWIN__) && \
     (__GNUC__ >= 3) && (__GNUC_MINOR__ >= 3) && !defined (_GLIBCPP_USE_C99)
#  define _STLP_NO_VENDOR_MATH_L
#  define _STLP_NO_VENDOR_STDLIB_L
#endif

#if (__GNUC__ < 3)
#  define _STLP_NO_VENDOR_STDLIB_L
#endif

#if (__GNUC__ < 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ < 4))

#  define _STLP_NO_MEMBER_TEMPLATE_KEYWORD
#endif

#if !defined (_REENTRANT) && (defined (_THREAD_SAFE) || \
                             (defined (__OpenBSD__) && defined (_POSIX_THREADS)) || \
                             (defined (__MINGW32__) && defined (_MT)))
#  define _REENTRANT
#endif

#if defined (__DJGPP)
#  define _STLP_RAND48    1
#  define _NOTHREADS    1
#  undef  _PTHREADS
#  define _STLP_LITTLE_ENDIAN
#endif

#if defined (__MINGW32__)

#  if (__GNUC__ >= 3)

#    define _STLP_VENDOR_GLOBAL_CSTD
#  endif
#  undef  _STLP_NO_DRAND48
#  define _STLP_NO_DRAND48
#  define _STLP_CALL
#endif 

#if defined (__CYGWIN__) || defined (__MINGW32__)
#  if !defined (_STLP_USE_STATIC_LIB)
#    define _STLP_USE_DECLSPEC 1
#    if !defined (_STLP_USE_DYNAMIC_LIB)
#      define _STLP_USE_DYNAMIC_LIB
#    endif
#    define _STLP_EXPORT_DECLSPEC __declspec(dllexport)
#    define _STLP_CLASS_EXPORT_DECLSPEC __declspec(dllexport)
#    define _STLP_CLASS_IMPORT_DECLSPEC __declspec(dllimport)
#  endif



#  define _STLP_IMPORT_DECLSPEC __declspec(dllimport)
#else
#  if (__GNUC__ >= 4)
#    if !defined (_STLP_USE_STATIC_LIB)
#      if !defined (_STLP_USE_DYNAMIC_LIB)
#        define _STLP_USE_DYNAMIC_LIB
#      endif
#      define _STLP_USE_DECLSPEC 1
#      define _STLP_EXPORT_DECLSPEC __attribute__((visibility("default")))
#      define _STLP_IMPORT_DECLSPEC __attribute__((visibility("default")))
#      define _STLP_CLASS_EXPORT_DECLSPEC __attribute__((visibility("default")))
#      define _STLP_CLASS_IMPORT_DECLSPEC __attribute__((visibility("default")))
#    endif
#  endif
#endif

#if defined (__CYGWIN__) || defined (__MINGW32__) || !(defined (_STLP_USE_GLIBC) || defined (__sun) || defined(__APPLE__))
#  if !defined (__MINGW32__) && !defined (__CYGWIN__)
#    define _STLP_NO_NATIVE_MBSTATE_T    1
#  endif
#  if !defined (__MINGW32__) || (__GNUC__ < 3) || (__GNUC__ == 3) && (__GNUC_MINOR__ < 4)
#    define _STLP_NO_NATIVE_WIDE_FUNCTIONS 1
#  endif
#  define _STLP_NO_NATIVE_WIDE_STREAMS   1
#endif

#define _STLP_NORETURN_FUNCTION __attribute__((noreturn))



#if defined (__APPLE__)
#  if ((__GNUC__ < 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ < 3)))

typedef unsigned int wint_t;
#  endif

#  define __unix

#  define _STLP_NO_LONG_DOUBLE


#  define _STLP_USE_NEW_C_HEADERS

#  define _STLP_NO_VENDOR_STDLIB_L

#endif 


#define _STLP_LONG_LONG long long

#ifdef _STLP_USE_UCLIBC
  
#  define _STLP_NO_VENDOR_MATH_F

#  define _STLP_NO_VENDOR_MATH_L
#  define _STLP_NO_LONG_DOUBLE
#endif

#if defined (__OpenBSD__) || defined (__FreeBSD__)
#  define _STLP_NO_VENDOR_MATH_L
#  define _STLP_NO_VENDOR_STDLIB_L
#  ifndef __unix
#    define __unix
#  endif
#endif

#if defined (__alpha__)
#  define _STLP_NO_VENDOR_MATH_L
#endif

#if defined (__hpux)
#  define _STLP_VENDOR_GLOBAL_CSTD 1
#  define _STLP_NO_VENDOR_STDLIB_L

#  define _STLP_NO_VENDOR_MATH_F
#endif

#if (__GNUC__ >= 3)
#  ifndef _STLP_HAS_NO_NEW_C_HEADERS



#    define _STLP_HAS_NATIVE_FLOAT_ABS



#  else
#    ifdef _STLP_USE_GLIBC
#      define _STLP_VENDOR_LONG_DOUBLE_MATH  1
#    endif
#  endif
#endif

#if (__GNUC__ < 3)
#  define _STLP_HAS_NO_NEW_C_HEADERS     1
#  define _STLP_VENDOR_GLOBAL_CSTD       1
#  define _STLP_DONT_USE_PTHREAD_SPINLOCK 1
#  ifndef __HONOR_STD
#    define _STLP_VENDOR_GLOBAL_EXCEPT_STD 1
#  endif

#  define _STLP_DEF_CONST_PLCT_NEW_BUG 1
#endif

#undef _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT
#undef _STLP_NO_UNEXPECTED_EXCEPT_SUPPORT


#if defined ( __STRICT_ANSI__ )
#  undef _STLP_LONG_LONG 
#endif

#ifndef __EXCEPTIONS
#  undef  _STLP_DONT_USE_EXCEPTIONS
#  define _STLP_DONT_USE_EXCEPTIONS 1
#endif

#if (__GNUC__ >= 3)



#  define _STLP_NO_FORCE_INSTANTIATE
#endif
