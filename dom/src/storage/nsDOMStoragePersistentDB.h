




#ifndef nsDOMStoragePersistentDB_h___
#define nsDOMStoragePersistentDB_h___

#include "nscore.h"
#include "nsDOMStorageBaseDB.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "nsTHashtable.h"
#include "nsDataHashtable.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/storage/StatementCache.h"

class DOMStorageImpl;
class nsSessionStorageEntry;

using mozilla::TimeStamp;
using mozilla::TimeDuration;

class nsDOMStoragePersistentDB : public nsDOMStorageBaseDB
{
  typedef mozilla::storage::StatementCache<mozIStorageStatement> StatementCache;

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
         bool aSecure);

  



  nsresult
  SetSecure(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            const bool aSecure);

  


  nsresult
  RemoveKey(DOMStorageImpl* aStorage,
            const nsAString& aKey);

  


  nsresult ClearStorage(DOMStorageImpl* aStorage);

  


  nsresult
  RemoveOwner(const nsACString& aOwner);

  


  nsresult
  RemoveAll();

  


  nsresult
  GetUsage(DOMStorageImpl* aStorage, int32_t *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, int32_t *aUsage);

  


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
  StatementCache mStatements;

  nsCString mCachedOwner;
  int32_t mCachedUsage;

  
  
  
  nsDataHashtable<nsCStringHashKey, TimeStamp> mTempTableLoads; 

  friend class nsDOMStorageDBWrapper;
  friend class nsDOMStorageMemoryDB;
  nsresult
  GetUsageInternal(const nsACString& aQuotaDBKey, int32_t *aUsage);

  
  
  bool DomainMaybeCached(const nsACString& aDomain);

};

#endif 
