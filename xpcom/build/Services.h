





#ifndef mozilla_Services_h
#define mozilla_Services_h

#include "nscore.h"
#include "nsCOMPtr.h"

#define MOZ_USE_NAMESPACE
#define MOZ_SERVICE(NAME, TYPE, SERVICE_CID) class TYPE;

#include "ServiceList.h"
#undef MOZ_SERVICE
#undef MOZ_USE_NAMESPACE

namespace mozilla {
namespace services {

#ifdef MOZILLA_INTERNAL_API
#define MOZ_SERVICE(NAME, TYPE, SERVICE_CID)                        \
    already_AddRefed<TYPE> Get##NAME();                             \
    NS_EXPORT_(already_AddRefed<TYPE>) _external_Get##NAME();

#include "ServiceList.h"
#undef MOZ_SERVICE
#else
#define MOZ_SERVICE(NAME, TYPE, SERVICE_CID)                        \
    NS_IMPORT_(already_AddRefed<TYPE>) _external_Get##NAME();       \
    inline already_AddRefed<TYPE> Get##NAME()                       \
    {                                                               \
        return _external_Get##NAME();                               \
    }

#include "ServiceList.h"
#undef MOZ_SERVICE
#endif

} 
} 

#endif
