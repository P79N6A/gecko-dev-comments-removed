



#ifndef mozilla_dom_workers_ServiceWorkerScriptCache_h
#define mozilla_dom_workers_ServiceWorkerScriptCache_h

#include "nsString.h"

class nsIPrincipal;

namespace mozilla {
namespace dom {
namespace workers {

namespace serviceWorkerScriptCache {

nsresult
PurgeCache(nsIPrincipal* aPrincipal, const nsAString& aCacheName);

nsresult
GenerateCacheName(nsAString& aName);

} 

} 
} 
} 

#endif 
