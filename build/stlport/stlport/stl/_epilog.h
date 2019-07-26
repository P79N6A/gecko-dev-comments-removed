





#ifndef _STLP_PROLOG_HEADER_INCLUDED
#  error STLport epilog header can not be included as long as prolog has not be included.
#endif




#if defined (_STLP_HAS_SPECIFIC_PROLOG_EPILOG)
#  include <stl/config/_epilog.h>
#endif

#if !defined (_STLP_NO_POST_COMPATIBLE_SECTION)
#  include <stl/_config_compat_post.h>
#endif

#if defined (_STLP_USE_OWN_NAMESPACE)

#  if !defined (_STLP_DONT_REDEFINE_STD)




#    if defined (std)





#      error Incompatible native Std library.
#    endif 
#    define std STLPORT
#  endif 

#endif

#undef _STLP_PROLOG_HEADER_INCLUDED /* defined in _prolog.h */
