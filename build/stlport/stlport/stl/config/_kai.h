

#if defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4)
# define _STLP_RAND48 1
#endif

#  ifndef __KAI_STRICT 
#   define _STLP_LONG_LONG long long
#  endif

#  if !defined (__EXCEPTIONS) && ! defined (_EXCEPTIONS)
#    define _STLP_HAS_NO_EXCEPTIONS
#  endif

# ifndef __BUILDING_STLPORT
#  define _STLP_LINK_TIME_INSTANTIATION 1
# endif


#   define _STLP_NATIVE_HEADER(header)    <../include/##header>
#   define _STLP_NATIVE_C_HEADER(header)    <../include/##header>
#   define _STLP_NATIVE_CPP_C_HEADER(header)    <../include/##header>
#   define _STLP_NATIVE_CPP_RUNTIME_HEADER(header) <../include/##header>




#  define _STLP_VENDOR_GLOBAL_CSTD 1
#  define _STLP_VENDOR_MB_NAMESPACE std


# if __KCC_VERSION < 4000
#  define _STLP_VENDOR_GLOBAL_EXCEPT_STD 1

# endif

# if defined (__sgi)


#  define _STLP_HAS_NO_NEW_C_HEADERS 1
#  include <standards.h>
# endif

