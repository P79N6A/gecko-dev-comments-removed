





































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
  nsDOMStorageMemoryDB() : mPreloading(PR_FALSE) {}
  ~nsDOMStorageMemoryDB() {}

  class nsInMemoryItem
  {
  public:
    PRBool mSecure;
    nsString mValue;
  };

  typedef nsClassHashtable<nsStringHashKey, nsInMemoryItem> nsStorageItemsTable;

  class nsInMemoryStorage
  {
  public:
    nsStorageItemsTable mTable;
    PRInt32 mUsageDelta;

    nsInMemoryStorage() : mUsageDelta(0) {}
  };

  





  nsresult
  Init(nsDOMStoragePersistentDB* aPreloadDB = nsnull);

  


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
              PRBool* aSecure);

  


  nsresult
  SetKey(DOMStorageImpl* aStorage,
         const nsAString& aKey,
         const nsAString& aValue,
         PRBool aSecure,
         PRInt32 aQuota,
         PRBool aExcludeOfflineFromUsage,
         PRInt32* aNewUsage);

  



  nsresult
  SetSecure(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            const PRBool aSecure);

  


  nsresult
  RemoveKey(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            PRBool aExcludeOfflineFromUsage,
            PRInt32 aKeyUsage);

  


  nsresult
  ClearStorage(DOMStorageImpl* aStorage);

  


  nsresult
  DropStorage(DOMStorageImpl* aStorage);

  


  nsresult
  RemoveOwner(const nsACString& aOwner, PRBool aIncludeSubDomains);

  



  nsresult
  RemoveOwners(const nsTArray<nsString>& aOwners,
               PRBool aIncludeSubDomains, PRBool aMatch);

  


  nsresult
  RemoveAll();

  


  nsresult
  GetUsage(DOMStorageImpl* aStorage, PRBool aExcludeOfflineFromUsage, PRInt32 *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, PRBool aIncludeSubDomains, PRInt32 *aUsage);

protected:

  nsClassHashtable<nsCStringHashKey, nsInMemoryStorage> mData;
  nsDOMStoragePersistentDB* mPreloadDB;
  PRBool mPreloading;

  nsresult
  GetUsageInternal(const nsACString& aQuotaDomainDBKey, PRBool aExcludeOfflineFromUsage, PRInt32 *aUsage);
};

#endif
