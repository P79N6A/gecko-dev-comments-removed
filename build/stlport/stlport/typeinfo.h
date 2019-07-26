


















#if defined (__DMC__)


#  define _STLP_OUTERMOST_HEADER_ID 0x874
#  include <../include/typeinfo.h>
#  undef _STLP_OUTERMOST_HEADER_ID
#else
#  ifndef _STLP_OLDSTD_typeinfo
#  define _STLP_OLDSTD_typeinfo

#  ifndef _STLP_OUTERMOST_HEADER_ID
#    define _STLP_OUTERMOST_HEADER_ID 0x874
#    include <stl/_prolog.h>
#  endif

#  ifndef _STLP_NO_TYPEINFO

#    if defined (__GNUC__)
#      undef _STLP_OLDSTD_typeinfo
#      include <typeinfo>
#      define _STLP_OLDSTD_typeinfo
#    else
#      if defined (_STLP_HAS_INCLUDE_NEXT)
#        include_next <typeinfo.h>
#      elif !defined (__BORLANDC__) || (__BORLANDC__ < 0x580)
#        include _STLP_NATIVE_CPP_RUNTIME_HEADER(typeinfo.h)
#      else
#        include _STLP_NATIVE_CPP_C_HEADER(typeinfo.h)
#      endif
#      if defined (__BORLANDC__) && (__BORLANDC__ >= 0x580) || \
          defined (__DMC__)
using std::type_info;
using std::bad_typeid;
using std::bad_cast;
#      endif
#    endif



#    if defined (_STLP_USE_OWN_NAMESPACE) && !(defined (_STLP_TYPEINFO) && !defined (_STLP_NO_NEW_NEW_HEADER))

_STLP_BEGIN_NAMESPACE

using  :: type_info;
#      if !(defined(__MRC__) || (defined(__SC__) && !defined(__DMC__)))
using  :: bad_typeid;
#      endif

using  :: bad_cast;

_STLP_END_NAMESPACE

#    endif 

#  endif 

#  if (_STLP_OUTERMOST_HEADER_ID == 0x874)
#    include <stl/_epilog.h>
#    undef _STLP_OUTERMOST_HEADER_ID
#  endif

#  endif 

#endif 




