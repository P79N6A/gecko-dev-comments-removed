





































#ifndef nsDOMOfflineResourceList_h___
#define nsDOMOfflineResourceList_h___

#include "nscore.h"
#include "nsIDOMOfflineResourceList.h"
#include "nsIOfflineCacheSession.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"

class nsDOMOfflineResourceList : public nsIDOMOfflineResourceList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMOFFLINERESOURCELIST

  nsDOMOfflineResourceList(nsIURI* aURI);
  virtual ~nsDOMOfflineResourceList();

private:
  nsresult Init();

  nsresult GetCacheKey(const nsAString &aURI, nsCString &aKey);
  nsresult GetCacheKey(nsIURI *aURI, nsCString &aKey);

  nsresult CacheKeys();
  void ClearCachedKeys();

  PRBool mInitialized;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIOfflineCacheSession> mCacheSession;
  nsCAutoString mHostPort;
};

#endif
