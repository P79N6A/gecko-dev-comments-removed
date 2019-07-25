











































#ifndef mozilla_Types_h_
#define mozilla_Types_h_













#include "jstypes.h"













#define MOZ_EXPORT_API(type_)  JS_EXPORT_API(type_)
#define MOZ_IMPORT_API(type_)  JS_IMPORT_API(type_)





#if defined(IMPL_MFBT)
# define MFBT_API(type_)       MOZ_EXPORT_API(type_)
#else
# define MFBT_API(type_)       MOZ_IMPORT_API(type_)
#endif


#define MOZ_BEGIN_EXTERN_C     JS_BEGIN_EXTERN_C
#define MOZ_END_EXTERN_C       JS_END_EXTERN_C

#ifdef __cplusplus









#if defined(__clang__)
# if __clang_major__ >= 3
#  define MOZ_HAVE_CXX11_DELETE
#  define MOZ_HAVE_CXX11_OVERRIDE
# elif __clang_major__ == 2
#  if __clang_minor__ >= 9
#   define MOZ_HAVE_CXX11_DELETE
#  endif
# endif
#elif defined(__GNUC__)
# if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#  if __GNUC__ > 4
#   define MOZ_HAVE_CXX11_DELETE
#   define MOZ_HAVE_CXX11_OVERRIDE
#  elif __GNUC__ == 4
#   if __GNUC_MINOR__ >= 7
#    define MOZ_HAVE_CXX11_OVERRIDE
#   endif
#   if __GNUC_MINOR__ >= 4
#    define MOZ_HAVE_CXX11_DELETE
#   endif
#  endif
# endif
#elif defined(_MSC_VER)
# if _MSC_VER >= 1400
#  define MOZ_HAVE_CXX11_OVERRIDE
# endif
#endif























#if defined(MOZ_HAVE_CXX11_DELETE)
# define MOZ_DELETE            = delete
#else
# define MOZ_DELETE
#endif




































#if defined(MOZ_HAVE_CXX11_OVERRIDE)
# define MOZ_OVERRIDE          override
#else
# define MOZ_OVERRIDE
#endif

#endif 

#endif  
