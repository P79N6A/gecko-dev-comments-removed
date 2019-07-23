





































#ifndef nsDOMStoragePersistentDB_h___
#define nsDOMStoragePersistentDB_h___

#include "nscore.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "nsTHashtable.h"

class nsDOMStorage;
class nsSessionStorageEntry;

class nsDOMStoragePersistentDB
{
public:
  nsDOMStoragePersistentDB() {}
  ~nsDOMStoragePersistentDB() {}

  nsresult
  Init();

  


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
         PRBool aExcludeOfflineFromUsage,
         PRInt32* aNewUsage);

  



  nsresult
  SetSecure(nsDOMStorage* aStorage,
            const nsAString& aKey,
            const PRBool aSecure);

  


  nsresult
  RemoveKey(nsDOMStorage* aStorage,
            const nsAString& aKey,
            PRBool aExcludeOfflineFromUsage,
            PRInt32 aKeyUsage);

  


  nsresult ClearStorage(nsDOMStorage* aStorage);

  


  nsresult
  RemoveOwner(const nsACString& aOwner, PRBool aIncludeSubDomains);

  



  nsresult
  RemoveOwners(const nsTArray<nsString>& aOwners,
               PRBool aIncludeSubDomains, PRBool aMatch);

  


  nsresult
  RemoveAll();

  


  nsresult
  GetUsage(nsDOMStorage* aStorage, PRBool aExcludeOfflineFromUsage, PRInt32 *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, PRBool aIncludeSubDomains, PRInt32 *aUsage);

  


  nsresult ClearAllPrivateBrowsingData();

protected:

  nsCOMPtr<mozIStorageConnection> mConnection;

  nsCOMPtr<mozIStorageStatement> mGetAllKeysStatement;
  nsCOMPtr<mozIStorageStatement> mGetKeyValueStatement;
  nsCOMPtr<mozIStorageStatement> mInsertKeyStatement;
  nsCOMPtr<mozIStorageStatement> mUpdateKeyStatement;
  nsCOMPtr<mozIStorageStatement> mSetSecureStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveKeyStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveOwnerStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveStorageStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveAllStatement;
  nsCOMPtr<mozIStorageStatement> mGetOfflineExcludedUsageStatement;
  nsCOMPtr<mozIStorageStatement> mGetFullUsageStatement;

  nsCString mCachedOwner;
  PRInt32 mCachedUsage;

  friend class nsDOMStorageDBWrapper;
  friend class nsDOMStorageMemoryDB;
  nsresult
  GetUsageInternal(const nsACString& aQuotaDomainDBKey, PRBool aExcludeOfflineFromUsage, PRInt32 *aUsage);
};

#endif 
