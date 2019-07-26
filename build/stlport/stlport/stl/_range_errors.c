













#if defined(_STLP_THROW_RANGE_ERRORS)
#  if defined (_STLP_WHOLE_NATIVE_STD) && defined (_STLP_DONT_REDEFINE_STD)



#    include <stdexcept>
#    include <string>
#    define _STLP_THROW_MSG(ex,msg)  throw std::ex(msg)
#  else
#    if defined (__BUILDING_STLPORT)
#      include <stdexcept>
#      include <string>
#    else
#      ifndef _STLP_INTERNAL_STDEXCEPT
#        include <stl/_stdexcept.h>
#      endif
#      ifndef _STLP_INTERNAL_STRING_H
#        include <stl/_string.h>
#      endif
#    endif
#    define _STLP_THROW_MSG(ex,msg)  throw ex(msg)
#  endif
#else
#  if defined (__BUILDING_STLPORT)
#    include <cstdlib>
#    include <cstdio>
#  else
#    ifndef _STLP_INTERNAL_CSTDLIB
#      include <stl/_cstdlib.h>
#    endif
#    ifndef _STLP_INTERNAL_CSTDIO
#      include <stl/_cstdio.h>
#    endif
#  endif
#  define _STLP_THROW_MSG(ex,msg)  puts(msg),_STLP_ABORT()
#endif




#if defined (_STLP_EXTERN_RANGE_ERRORS)
#  define _STLP_THROW_FUNCT_SPEC void _STLP_DECLSPEC
#else
#  define _STLP_THROW_FUNCT_SPEC inline void
#endif

_STLP_BEGIN_NAMESPACE

_STLP_THROW_FUNCT_SPEC _STLP_CALL __stl_throw_runtime_error(const char* __msg)
{ _STLP_THROW_MSG(runtime_error, __msg); }

_STLP_THROW_FUNCT_SPEC _STLP_CALL __stl_throw_range_error(const char* __msg)
{ _STLP_THROW_MSG(range_error, __msg); }

_STLP_THROW_FUNCT_SPEC _STLP_CALL __stl_throw_out_of_range(const char* __msg)
{ _STLP_THROW_MSG(out_of_range, __msg); }

_STLP_THROW_FUNCT_SPEC _STLP_CALL __stl_throw_length_error(const char* __msg)
{ _STLP_THROW_MSG(length_error, __msg); }

_STLP_THROW_FUNCT_SPEC _STLP_CALL __stl_throw_invalid_argument(const char* __msg)
{ _STLP_THROW_MSG(invalid_argument, __msg); }

_STLP_THROW_FUNCT_SPEC _STLP_CALL __stl_throw_overflow_error(const char* __msg)
{ _STLP_THROW_MSG(overflow_error, __msg); }

_STLP_END_NAMESPACE

#undef _STLP_THROW_FUNCT_SPEC
#undef _STLP_THROW_MSG
