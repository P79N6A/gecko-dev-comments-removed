

#define _STLP_COMPILER __DMC_VERSION_STRING__

#if (__DMC__ < 0x849)
#  error "Digital Mars C++ compilers before version 8.49 are not supported!"
#endif





#define _STLP_NO_CONST_IN_PAIR
#define _STLP_DONT_SUP_DFLT_PARAM

#ifndef _CPPUNWIND
#  define _STLP_NO_EXCEPTIONS
#endif

#ifndef _CPPRTTI
#  define _STLP_NO_RTTI
#endif

#define _STLP_VENDOR_GLOBAL_CSTD



#define _STLP_STATIC_CONST_INIT_BUG

#if !defined (_WIN32)

#  define _STLP_NO_NATIVE_WIDE_FUNCTIONS
#endif

















#define _STLP_NO_OWN_NAMESPACE 1


#if defined (_MT) && !defined (_STLP_NO_THREADS)
#  define _STLP_THREADS
#endif

#ifndef _BOOL_DEFINED
#  define _STLP_NO_BOOL
#else
#  define _STLP_DONT_USE_BOOL_TYPEDEF
#endif

#if _INTEGRAL_MAX_BITS >= 64
#  define _STLP_LONG_LONG long long
#endif

#define _STLP_MARK_PARAMETER_AS_UNUSED(X)
#define _STLP_DONT_USE_PRIV_NAMESPACE
#define _STLP_PRIV
#define _STLP_THROW_RETURN_BUG

#if !defined (_DLL)
#  undef _STLP_NO_UNEXPECTED_EXCEPT_SUPPORT
#endif

#if (__DMC__ < 0x849)
#  define _STLP_NO_BAD_ALLOC
#endif

#define _STLP_USE_ABBREVS
#define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER

#define _STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND
#define _STLP_EXPORT_DECLSPEC __declspec(dllexport)
#define _STLP_IMPORT_DECLSPEC __declspec(dllimport)

#define _STLP_CLASS_EXPORT_DECLSPEC __declspec(dllexport)
#define _STLP_CLASS_IMPORT_DECLSPEC __declspec(dllimport)

#define _STLP_NEED_ADDITIONAL_STATIC_DECLSPEC




#if defined (_WINDLL)
#  define _STLP_DLL
#endif
#if defined (_DLL)
#  define _STLP_RUNTIME_DLL
#endif
#include <stl/config/_detect_dll_or_lib.h>
#undef _STLP_RUNTIME_DLL
#undef _STLP_DLL

#if defined (_STLP_USE_DYNAMIC_LIB)
#  define _STLP_USE_DECLSPEC 1
#  if defined (__BUILDING_STLPORT)
#    define _STLP_CALL __export
#  else
#    define _STLP_CALL
#  endif
#else
#  define _STLP_CALL
#endif

#include <stl/config/_auto_link.h>

#undef __SC__

#include <stl/config/_feedback.h>
