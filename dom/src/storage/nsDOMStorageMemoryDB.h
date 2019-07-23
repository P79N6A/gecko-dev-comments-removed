





































#ifndef nsDOMStorageMemoryDB_h___
#define nsDOMStorageMemoryDB_h___

#include "nscore.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"

class nsDOMStoragePersistentDB;

class nsDOMStorageMemoryDB
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
  GetItemsTable(nsDOMStorage* aStorage,
                nsInMemoryStorage** aMemoryStorage);

  


  nsresult
  GetAllKeys(nsDOMStorage* aStorage,
             nsTHashtable<nsSessionStorageEntry>* aKeys);

  




  nsresult
  GetKeyValue(nsDOMStorage* aStorage,
              const nsAString& aKey,
              nsAString& aValue,
              PRBool* aSecure);

  


  nsresult
  SetKey(nsDOMStorage* aStorage,
         const nsAString& aKey,
         const nsAString& aValue,
         PRBool aSecure,
         PRInt32 aQuota,
         PRInt32* aNewUsage);

  



  nsresult
  SetSecure(nsDOMStorage* aStorage,
            const nsAString& aKey,
            const PRBool aSecure);

  


  nsresult
  RemoveKey(nsDOMStorage* aStorage,
            const nsAString& aKey,
            PRInt32 aKeyUsage);

  


  nsresult
  ClearStorage(nsDOMStorage* aStorage);

  


  nsresult
  DropStorage(nsDOMStorage* aStorage);

  


  nsresult
  RemoveOwner(const nsACString& aOwner, PRBool aIncludeSubDomains);

  



  nsresult
  RemoveOwners(const nsTArray<nsString>& aOwners,
               PRBool aIncludeSubDomains, PRBool aMatch);

  


  nsresult
  RemoveAll();

  


  nsresult
  GetUsage(nsDOMStorage* aStorage, PRInt32 *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, PRBool aIncludeSubDomains, PRInt32 *aUsage);

protected:

  nsClassHashtable<nsCStringHashKey, nsInMemoryStorage> mData;
  nsDOMStoragePersistentDB* mPreloadDB;
  PRBool mPreloading;

  nsresult
  GetUsageInternal(const nsACString& aQuotaDomainDBKey, PRInt32 *aUsage);
};

#endif
