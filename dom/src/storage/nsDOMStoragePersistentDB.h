




#ifndef nsDOMStoragePersistentDB_h___
#define nsDOMStoragePersistentDB_h___

#include "nscore.h"
#include "nsDOMStorageBaseDB.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozilla/storage/StatementCache.h"
#include "mozIStorageBindingParamsArray.h"
#include "nsTHashtable.h"
#include "nsDataHashtable.h"
#include "nsIThread.h"

#include "nsLocalStorageCache.h"

class DOMStorageImpl;
class nsSessionStorageEntry;

class nsDOMStoragePersistentDB : public nsDOMStorageBaseDB
{
  typedef mozilla::storage::StatementCache<mozIStorageStatement> StatementCache;
  typedef nsLocalStorageCache::FlushData FlushData;

public:
  nsDOMStoragePersistentDB();
  ~nsDOMStoragePersistentDB() {}

  nsresult
  Init(const nsString& aDatabaseName);

  


  void
  Close();

  



  bool
  IsFlushTimerNeeded() const;

  


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
  RemoveOwner(const nsACString& aOwner);

  


  nsresult
  RemoveAll();

  





  nsresult
  RemoveAllForApp(uint32_t aAppId, bool aOnlyBrowserElement);

  


  nsresult
  GetUsage(DOMStorageImpl* aStorage, int32_t *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, int32_t *aUsage);

  


  nsresult
  ClearAllPrivateBrowsingData();

  















  nsresult
  FlushAndEvictFromCache(bool aMainThread);

  


  nsresult
  Flush();

  



  void
  HandleFlushComplete(bool aSucceeded);

private:

  friend class nsDOMStorageMemoryDB;

  


  nsresult
  SetJournalMode(bool aIsWal);

  


  nsresult
  ConfigureWalBehavior();

  


  nsresult
  EnsureScopeLoaded(DOMStorageImpl* aStorage);

  


  nsresult
  FetchScope(DOMStorageImpl* aStorage, nsScopeCache* aScopeCache);

  







  nsresult
  EnsureQuotaUsageLoaded(const nsACString& aQuotaKey);

  


  nsresult
  FetchQuotaUsage(const nsACString& aQuotaDBKey);

  nsresult
  GetUsageInternal(const nsACString& aQuotaDBKey, int32_t *aUsage);

  


  nsresult
  FetchMatchingScopeNames(const nsACString& aPattern);

  nsresult
  PrepareFlushStatements(const FlushData& aFlushData);

  



  nsresult
  PrepareForFlush();

  


  void
  EvictUnusedScopes();

  


  nsTArray<nsCOMPtr<mozIStorageStatement> > mFlushStatements;
  nsTArray<nsCOMPtr<mozIStorageBindingParamsArray> > mFlushStatementParams;
  StatementCache mReadStatements;
  StatementCache mWriteStatements;
  nsCOMPtr<mozIStorageConnection> mReadConnection;
  nsCOMPtr<mozIStorageConnection> mWriteConnection;

  


  nsLocalStorageCache mCache;
  nsDataHashtable<nsCStringHashKey, int32_t> mQuotaUseByUncached;
  
  bool mWasRemoveAllCalled;
  
  bool mIsRemoveAllPending;
  
  bool mIsFlushPending;

  
  nsCOMPtr<nsIThread> mFlushThread;
};

#endif 
