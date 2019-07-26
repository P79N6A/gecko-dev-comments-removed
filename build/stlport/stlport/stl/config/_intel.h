


#define _STLP_COMPILER "Intel ICL"

#define _STLP_IMPORT_TEMPLATE_KEYWORD extern





















#include <stl/config/_msvc.h>

#if defined (_STLP_DONT_RETURN_VOID)
#  undef _STLP_DONT_RETURN_VOID
#endif

#if (__ICL < 900)
#  define _STLP_NOTHROW
#endif

#if (__ICL <= 810)



#  define _STLP_NO_METHOD_SPECIALIZATION 1
#endif

#if (__ICL >= 800 && __ICL < 900)
#  define _STLP_STATIC_CONST_INIT_BUG 1
#endif

#if (__ICL >= 450)
#  define _STLP_DLLEXPORT_NEEDS_PREDECLARATION 1
#endif

#if (__ICL < 450)

#  undef  _STLP_USE_STATIC_LIB
#  undef  _STLP_USE_DYNAMIC_LIB
#  define _STLP_USE_STATIC_LIB

#  undef _STLP_NO_CUSTOM_IO
#endif

#undef  _STLP_LONG_LONG
#define _STLP_LONG_LONG long long

#if defined (__cplusplus) && (__ICL >= 900) && (_STLP_MSVC_LIB < 1300)
namespace std
{
  void _STLP_CALL unexpected();
}
#endif

#include <stl/config/_feedback.h>
