




#ifndef nsDOMStorageManager_h__
#define nsDOMStorageManager_h__

#include "nsIDOMStorageManager.h"
#include "DOMStorageObserver.h"

#include "DOMStorageCache.h"
#include "mozilla/dom/DOMStorage.h"

#include "nsTHashtable.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"

class nsIDOMWindow;

namespace mozilla {
namespace dom {

const DOMStorage::StorageType SessionStorage = DOMStorage::SessionStorage;
const DOMStorage::StorageType LocalStorage = DOMStorage::LocalStorage;

class DOMStorageManager : public nsIDOMStorageManager
                        , public DOMStorageObserverSink
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSTORAGEMANAGER

public:
  virtual DOMStorage::StorageType Type() { return mType; }

  
  static uint32_t GetQuota();
  
  DOMStorageCache* GetCache(const nsACString& aScope) const;
  
  already_AddRefed<DOMStorageUsage> GetScopeUsage(const nsACString& aScope);

protected:
  DOMStorageManager(DOMStorage::StorageType aType);
  virtual ~DOMStorageManager();

private:
  
  virtual nsresult Observe(const char* aTopic, const nsACString& aScopePrefix);

  
  
  class DOMStorageCacheHashKey : public nsCStringHashKey
  {
  public:
    DOMStorageCacheHashKey(const nsACString* aKey)
      : nsCStringHashKey(aKey)
      , mCache(new DOMStorageCache(aKey))
    {}

    DOMStorageCacheHashKey(const DOMStorageCacheHashKey& aOther)
      : nsCStringHashKey(aOther)
    {
      NS_ERROR("Shouldn't be called");
    }

    DOMStorageCache* cache() { return mCache; }
    
    void HardRef() { mCacheRef = mCache; }

  private:
    
    DOMStorageCache* mCache;
    
    nsRefPtr<DOMStorageCache> mCacheRef;
  };

  
  
  already_AddRefed<DOMStorageCache> PutCache(const nsACString& aScope,
                                             nsIPrincipal* aPrincipal);

  
  nsresult GetStorageInternal(bool aCreate,
                              nsIDOMWindow* aWindow,
                              nsIPrincipal* aPrincipal,
                              const nsAString& aDocumentURI,
                              bool aPrivate,
                              nsIDOMStorage** aRetval);

  
  nsTHashtable<DOMStorageCacheHashKey> mCaches;
  const DOMStorage::StorageType mType;

  
  
  
  bool mLowDiskSpace;
  bool IsLowDiskSpace() const { return mLowDiskSpace; };

  static PLDHashOperator ClearCacheEnumerator(DOMStorageCacheHashKey* aCache,
                                              void* aClosure);

protected:
  
  nsDataHashtable<nsCStringHashKey, nsRefPtr<DOMStorageUsage> > mUsages;

  friend class DOMStorageCache;
  
  virtual void DropCache(DOMStorageCache* aCache);
};







class DOMLocalStorageManager MOZ_FINAL : public DOMStorageManager
{
public:
  DOMLocalStorageManager();
  virtual ~DOMLocalStorageManager();

  
  static DOMLocalStorageManager* Self() { return sSelf; }

private:
  static DOMLocalStorageManager* sSelf;
};

class DOMSessionStorageManager MOZ_FINAL : public DOMStorageManager
{
public:
  DOMSessionStorageManager();
};

} 
} 

#endif 
