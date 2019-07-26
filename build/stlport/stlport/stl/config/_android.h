#ifndef __stl_config__android_h
#define __stl_config__android_h

#define _STLP_PLATFORM "Android"


#define _STLP_UNIX 1


#define _PTHREADS


#define _STLP_HAS_NO_NEW_C_HEADERS 1


#define _STLP_USE_UNIX_IO 1


#undef _STLP_NO_RTTI


#define _STLP_VENDOR_GLOBAL_CSTD 1


#undef _STLP_REAL_LOCALE_IMPLEMENTED


#define _STLP_DONT_USE_PTHREAD_SPINLOCK 1


#undef _NOTHREADS


#define _STLP_LITTLE_ENDIAN 1


#undef _STLP_NO_EXCEPTION_HEADER


#undef _STLP_NO_EXCEPTIONS



#define _STLP_NO_OWN_NAMESPACE 1


#define _STLP_USE_SIMPLE_NODE_ALLOC 1



#define _STLP_USE_NO_EXTERN_RANGE_ERRORS 1



#define _STLP_NO_VENDOR_MATH_L 1


#define _STLP_NATIVE_HEADER(header) <usr/include/header>
#define _STLP_NATIVE_C_HEADER(header) <../include/header>
#define _STLP_NATIVE_CPP_C_HEADER(header) <../../gabi++/include/header>
#define _STLP_NATIVE_CPP_RUNTIME_HEADER(header) <../../gabi++/include/header>
#define _STLP_NATIVE_OLD_STREAMS_HEADER(header) <usr/include/header>


#include <stl/config/_gcc.h>


#undef _STLP_USE_GLIBC


#undef _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT
#undef _STLP_NO_UNEXPECTED_EXCEPT_SUPPORT

#ifndef _ANDROID_NDK_BLAZE_

#undef _STLP_HAS_INCLUDE_NEXT
#endif

#endif 
