



#ifndef AppCacheStorage__h__
#define AppCacheStorage__h__

#include "CacheStorage.h"

#include "nsCOMPtr.h"
#include "nsILoadContextInfo.h"
#include "nsIApplicationCache.h"

class nsIApplicationCache;

namespace mozilla {
namespace net {

class AppCacheStorage : public CacheStorage
{
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICACHESTORAGE

public:
  AppCacheStorage(nsILoadContextInfo* aInfo,
                  nsIApplicationCache* aAppCache);

private:
  virtual ~AppCacheStorage();

  nsCOMPtr<nsIApplicationCache> mAppCache;
};

} 
} 

#endif
