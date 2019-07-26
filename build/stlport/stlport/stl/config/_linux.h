#ifndef __stl_config__linux_h
#define __stl_config__linux_h

#define _STLP_PLATFORM "Linux"

#include <features.h>






#ifndef _STLP_USE_GLIBC
#  define _STLP_USE_GLIBC 1
#endif

#ifndef _STLP_USE_STDIO_IO
#  define _STLP_USE_UNIX_IO
#endif





#if !defined(_STLP_NO_THREADS) && !defined(_REENTRANT)
#  define _REENTRANT
#endif

#if defined(_REENTRANT) && !defined(_PTHREADS)
# define _PTHREADS
#endif

#ifdef __UCLIBC__ 
#  define _STLP_USE_UCLIBC 1
#  if !defined(__UCLIBC_HAS_WCHAR__)
#    ifndef _STLP_NO_WCHAR_T
#      define _STLP_NO_WCHAR_T
#    endif
#    ifndef _STLP_NO_NATIVE_MBSTATE_T
#      define _STLP_NO_NATIVE_MBSTATE_T
#    endif
#    ifndef _STLP_NO_NATIVE_WIDE_STREAMS
#      define _STLP_NO_NATIVE_WIDE_STREAMS
#    endif
#  endif 
   
#  define _STLP_VENDOR_GLOBAL_CSTD 1
#endif


#if defined(_PTHREADS)
#  define _STLP_THREADS
#  define _STLP_PTHREADS






 
#  ifdef __USE_XOPEN2K

#   ifndef __UCLIBC__ 
#     define _STLP_USE_PTHREAD_SPINLOCK
#   else
#     ifndef _STLP_DONT_USE_PTHREAD_SPINLOCK
        

#       define _STLP_DONT_USE_PTHREAD_SPINLOCK
#     endif
#   endif
#   ifndef _STLP_DONT_USE_PTHREAD_SPINLOCK
#     define _STLP_USE_PTHREAD_SPINLOCK
#     define _STLP_STATIC_MUTEX _STLP_mutex
#   endif

#  endif
#endif


#include <endian.h>
#if !defined(__BYTE_ORDER) || !defined(__LITTLE_ENDIAN) || !defined(__BIG_ENDIAN)
#  error "One of __BYTE_ORDER, __LITTLE_ENDIAN and __BIG_ENDIAN undefined; Fix me!"
#endif

#if ( __BYTE_ORDER == __LITTLE_ENDIAN )
#  define _STLP_LITTLE_ENDIAN 1
#elif ( __BYTE_ORDER == __BIG_ENDIAN )
#  define _STLP_BIG_ENDIAN 1
#else
#  error "__BYTE_ORDER neither __BIG_ENDIAN nor __LITTLE_ENDIAN; Fix me!"
#endif

#if defined(__GNUC__) && (__GNUC__ < 3)
#  define _STLP_NO_NATIVE_WIDE_FUNCTIONS 1
#endif

#ifdef __GLIBC__
#  if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 3) || (__GLIBC__ > 2)

#    if !defined(_STLP_USE_MALLOC) && !defined(_STLP_USE_NEWALLOC) && !defined(_STLP_USE_PERTHREAD_ALLOC) && !defined(_STLP_USE_NODE_ALLOC)
#      define _STLP_USE_MALLOC 1
#    endif
#  endif








#  if defined(__alpha__) || \
      defined(__ppc__) || defined(PPC) || defined(__powerpc__) || \
      ((defined(__sparc) || defined(__sparcv9) || defined(__sparcv8plus)) && !defined ( __WORD64 ) && !defined(__arch64__))  || \
      (defined(_MIPS_SIM) && (_MIPS_SIM == _ABIO32)) || \
      defined(__arm__) || \
      defined(__sh__)
 
#    define _STLP_NO_LONG_DOUBLE
#  endif
#endif


#endif 
