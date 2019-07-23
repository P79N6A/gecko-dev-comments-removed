





































#ifndef psSharedCore_h__
#define psSharedCore_h__

#include "nscore.h"

#ifdef MOZ_ENABLE_LIBXUL
#define NS_PSSHARED
#define NS_PSSHARED_(type) type
#define NS_PSSHARED_STATIC_MEMBER_(type) type
#else 
#ifdef _IMPL_NS_PSSHARED
#define NS_PSSHARED NS_EXPORT
#define NS_PSSHARED_(type) NS_EXPORT_(type)
#define NS_PSSHARED_STATIC_MEMBER_(type) NS_EXPORT_STATIC_MEMBER_(type)
#else
#define NS_PSSHARED NS_IMPORT
#define NS_PSSHARED_(type) NS_IMPORT_(type)
#define NS_PSSHARED_STATIC_MEMBER_(type) NS_IMPORT_STATIC_MEMBER_(type)
#endif
#endif 

#endif
