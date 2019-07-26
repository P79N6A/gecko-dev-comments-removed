




#ifndef nsDOMStorageMemoryDB_h___
#define nsDOMStorageMemoryDB_h___

#include "nscore.h"
#include "nsDOMStorageBaseDB.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"

class nsDOMStoragePersistentDB;

class nsDOMStorageMemoryDB : public nsDOMStorageBaseDB
{
public:
  nsDOMStorageMemoryDB() : mPreloading(false) {}
  ~nsDOMStorageMemoryDB() {}

  class nsInMemoryItem
  {
  public:
    bool mSecure;
    nsString mValue;
  };

  typedef nsClassHashtable<nsStringHashKey, nsInMemoryItem> nsStorageItemsTable;

  class nsInMemoryStorage
  {
  public:
    nsStorageItemsTable mTable;
    int32_t mUsageDelta;

    nsInMemoryStorage() : mUsageDelta(0) {}
  };

  





  nsresult
  Init(nsDOMStoragePersistentDB* aPreloadDB = nullptr);

  


  nsresult
  GetItemsTable(DOMStorageImpl* aStorage,
                nsInMemoryStorage** aMemoryStorage);

  


  nsresult
  GetAllKeys(DOMStorageImpl* aStorage,
             nsTHashtable<nsSessionStorageEntry>* aKeys);

  




  nsresult
  GetKeyValue(DOMStorageImpl* aStorage,
              const nsAString& aKey,
              nsAString& aValue,
              bool* aSecure);

  


  nsresult
  SetKey(DOMStorageImpl* aStorage,
         const nsAString& aKey,
         const nsAString& aValue,
         bool aSecure);

  



  nsresult
  SetSecure(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            const bool aSecure);

  


  nsresult
  RemoveKey(DOMStorageImpl* aStorage,
            const nsAString& aKey);

  


  nsresult
  ClearStorage(DOMStorageImpl* aStorage);

  


  nsresult
  DropStorage(DOMStorageImpl* aStorage);

  


  nsresult
  RemoveOwner(const nsACString& aOwner);

  


  nsresult
  RemoveAll();

  


  nsresult
  GetUsage(DOMStorageImpl* aStorage, int32_t *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, int32_t *aUsage);

protected:

  nsClassHashtable<nsCStringHashKey, nsInMemoryStorage> mData;
  nsDOMStoragePersistentDB* mPreloadDB;
  bool mPreloading;

  nsresult
  GetUsageInternal(const nsACString& aQuotaDBKey, int32_t *aUsage);
};

#endif
