











































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























#if defined(__clang__) && (__clang_major__ >= 3 || (__clang_major__ == 2 && __clang_minor__ >= 9))
# define MOZ_DELETE            = delete
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
 






# if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#  define MOZ_DELETE           = delete
# else
#  define MOZ_DELETE
# endif
#else
# define MOZ_DELETE
#endif

#endif 

#endif  
