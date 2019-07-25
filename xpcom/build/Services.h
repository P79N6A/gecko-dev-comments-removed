





































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

#define MOZ_SERVICE(NAME, TYPE, SERVICE_CID) NS_COM already_AddRefed<TYPE> Get##NAME();
#include "ServiceList.h"
#undef MOZ_SERVICE

} 
} 

#endif
