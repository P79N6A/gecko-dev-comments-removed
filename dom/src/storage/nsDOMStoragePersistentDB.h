





































#ifndef nsDOMStoragePersistentDB_h___
#define nsDOMStoragePersistentDB_h___

#include "nscore.h"
#include "nsDOMStorageBaseDB.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "nsTHashtable.h"
#include "nsDataHashtable.h"
#include "mozilla/TimeStamp.h"

class DOMStorageImpl;
class nsSessionStorageEntry;

using mozilla::TimeStamp;
using mozilla::TimeDuration;

class nsDOMStoragePersistentDB : public nsDOMStorageBaseDB
{
public:
  nsDOMStoragePersistentDB();
  ~nsDOMStoragePersistentDB() {}

  nsresult
  Init(const nsString& aDatabaseName);

  


  void
  Close();

  


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
         bool aSecure,
         PRInt32 aQuota,
         bool aExcludeOfflineFromUsage,
         PRInt32* aNewUsage);

  



  nsresult
  SetSecure(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            const bool aSecure);

  


  nsresult
  RemoveKey(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            bool aExcludeOfflineFromUsage,
            PRInt32 aKeyUsage);

  


  nsresult ClearStorage(DOMStorageImpl* aStorage);

  


  nsresult
  RemoveOwner(const nsACString& aOwner, bool aIncludeSubDomains);

  



  nsresult
  RemoveOwners(const nsTArray<nsString>& aOwners,
               bool aIncludeSubDomains, bool aMatch);

  


  nsresult
  RemoveAll();

  


  nsresult
  GetUsage(DOMStorageImpl* aStorage, bool aExcludeOfflineFromUsage, PRInt32 *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, bool aIncludeSubDomains, PRInt32 *aUsage);

  


  nsresult ClearAllPrivateBrowsingData();

  



  nsresult EnsureInsertTransaction();

  


  nsresult MaybeCommitInsertTransaction();

  


  nsresult FlushTemporaryTables(bool force);

protected:
  



  nsresult EnsureLoadTemporaryTableForStorage(DOMStorageImpl* aStorage);

  struct FlushTemporaryTableData {
    nsDOMStoragePersistentDB* mDB;
    bool mForce;
    nsresult mRV;
  };
  static PLDHashOperator FlushTemporaryTable(nsCStringHashKey::KeyType aKey,
                                             TimeStamp& aData,
                                             void* aUserArg);       

  nsCOMPtr<mozIStorageConnection> mConnection;

  nsCOMPtr<mozIStorageStatement> mCopyToTempTableStatement;
  nsCOMPtr<mozIStorageStatement> mCopyBackToDiskStatement;
  nsCOMPtr<mozIStorageStatement> mDeleteTemporaryTableStatement;
  nsCOMPtr<mozIStorageStatement> mGetAllKeysStatement;
  nsCOMPtr<mozIStorageStatement> mGetKeyValueStatement;
  nsCOMPtr<mozIStorageStatement> mInsertKeyStatement;
  nsCOMPtr<mozIStorageStatement> mSetSecureStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveKeyStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveOwnerStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveStorageStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveAllStatement;
  nsCOMPtr<mozIStorageStatement> mGetOfflineExcludedUsageStatement;
  nsCOMPtr<mozIStorageStatement> mGetFullUsageStatement;
  

  nsCString mCachedOwner;
  PRInt32 mCachedUsage;

  
  
  
  nsDataHashtable<nsCStringHashKey, TimeStamp> mTempTableLoads; 

  friend class nsDOMStorageDBWrapper;
  friend class nsDOMStorageMemoryDB;
  nsresult
  GetUsageInternal(const nsACString& aQuotaDomainDBKey, bool aExcludeOfflineFromUsage, PRInt32 *aUsage);
};

#endif 
