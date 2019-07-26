














#ifndef _STLP_INTERNAL_MBSTATE_T
#define _STLP_INTERNAL_MBSTATE_T

#if (defined (__OpenBSD__) || defined (__FreeBSD__) || defined (__hpux)) && defined (__GNUC__) && !defined (_GLIBCPP_HAVE_MBSTATE_T)
#  define _STLP_CPP_MBSTATE_T
#endif

#if defined (_STLP_NO_NATIVE_MBSTATE_T) || defined (_STLP_CPP_MBSTATE_T) && !defined (_MBSTATE_T)
#  define _STLP_USE_OWN_MBSTATE_T
#  define _MBSTATE_T
#endif

#if defined (_STLP_USE_OWN_MBSTATE_T)
#  if !defined (_STLP_CPP_MBSTATE_T) || !defined (__cplusplus) || !defined (_STLP_USE_NEW_C_HEADERS)
#    if !defined (__ANDROID__) 
typedef int mbstate_t;
#    endif
#  endif

#  if !defined (_STLP_CPP_MBSTATE_T) && defined (__cplusplus) && defined (_STLP_USE_NAMESPACES)
_STLP_BEGIN_NAMESPACE
using ::mbstate_t;
_STLP_END_NAMESPACE
#  endif

#endif 

#endif 
