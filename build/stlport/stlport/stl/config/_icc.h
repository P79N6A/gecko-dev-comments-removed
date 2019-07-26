



#define _STLP_COMPILER "Intel ICC"

#define _STLP_LONG_LONG long long




#if (__INTEL_COMPILER < 800)
# define _STLP_NATIVE_INCLUDE_PATH ../include
# define _STLP_NATIVE_C_INCLUDE_PATH ../include
# define _STLP_NATIVE_CPP_C_INCLUDE_PATH ../include
#endif

#if (__INTEL_COMPILER >= 800)
# define _STLP_NATIVE_INCLUDE_PATH ../include/c++
# define _STLP_NATIVE_C_INCLUDE_PATH ../include
# define _STLP_NATIVE_CPP_C_INCLUDE_PATH ../include

#endif 

#define _STLP_HAS_NO_NEW_C_HEADERS 1
#define _STLP_VENDOR_GLOBAL_CSTD 1


#if !defined (_STLP_USE_GLIBC) && defined (__linux__)
# define _STLP_USE_GLIBC
# define _XOPEN_SOURCE 600
#endif

#undef _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT


#ifndef __GNUC__
# define __GNUC__ 3
#endif



#define _STLP_NO_FORCE_INSTANTIATE

