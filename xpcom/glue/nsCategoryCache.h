





#ifndef nsCategoryCache_h_
#define nsCategoryCache_h_

#include "mozilla/Attributes.h"

#include "nsICategoryManager.h"
#include "nsIObserver.h"
#include "nsISimpleEnumerator.h"
#include "nsISupportsPrimitives.h"

#include "nsServiceManagerUtils.h"

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsInterfaceHashtable.h"

#include "nsXPCOM.h"

class nsCategoryObserver final : public nsIObserver
{
  ~nsCategoryObserver();

public:
  explicit nsCategoryObserver(const char* aCategory);

  void ListenerDied();
  nsInterfaceHashtable<nsCStringHashKey, nsISupports>& GetHash()
  {
    return mHash;
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
private:
  void RemoveObservers();

  nsInterfaceHashtable<nsCStringHashKey, nsISupports> mHash;
  nsCString mCategory;
  bool mObserversRemoved;
};








template<class T>
class nsCategoryCache final
{
public:
  explicit nsCategoryCache(const char* aCategory)
    : mCategoryName(aCategory)
  {
  }
  ~nsCategoryCache()
  {
    if (mObserver) {
      mObserver->ListenerDied();
    }
  }

  void GetEntries(nsCOMArray<T>& aResult)
  {
    
    
    if (!mObserver) {
      mObserver = new nsCategoryObserver(mCategoryName.get());
    }

    for (auto iter = mObserver->GetHash().Iter(); !iter.Done(); iter.Next()) {
      nsISupports* entry = iter.UserData();
      nsCOMPtr<T> service = do_QueryInterface(entry);
      if (service) {
        aResult.AppendObject(service);
      }
    }
  }

private:
  
  nsCategoryCache(const nsCategoryCache<T>&);

  nsCString mCategoryName;
  nsRefPtr<nsCategoryObserver> mObserver;
};

#endif
