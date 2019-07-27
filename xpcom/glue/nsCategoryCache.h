





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

class nsCategoryObserver MOZ_FINAL : public nsIObserver
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
class nsCategoryCache MOZ_FINAL
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

    mObserver->GetHash().EnumerateRead(EntriesToArray, &aResult);
  }

private:
  
  nsCategoryCache(const nsCategoryCache<T>&);

  static PLDHashOperator EntriesToArray(const nsACString& aKey,
                                        nsISupports* aEntry, void* aArg)
  {
    nsCOMArray<T>& entries = *static_cast<nsCOMArray<T>*>(aArg);

    nsCOMPtr<T> service = do_QueryInterface(aEntry);
    if (service) {
      entries.AppendObject(service);
    }
    return PL_DHASH_NEXT;
  }

  nsCString mCategoryName;
  nsRefPtr<nsCategoryObserver> mObserver;

};

#endif
