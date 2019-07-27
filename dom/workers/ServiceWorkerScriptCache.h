



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

class CompareCallback
{
public:
  virtual void ComparisonResult(nsresult aStatus, bool aInCacheAndEqual) = 0;

  virtual MozExternalRefCountType AddRef() = 0;
  virtual MozExternalRefCountType Release() = 0;
};

nsresult
Compare(nsIPrincipal* aPrincipal, const nsAString& aCacheName,
        const nsAString& aURL, CompareCallback* aCallback);

} 

} 
} 
} 

#endif 
