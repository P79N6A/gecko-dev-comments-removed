











































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

#endif  
