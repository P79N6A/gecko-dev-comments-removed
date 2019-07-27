




#ifndef DOMStorageDBThread_h___
#define DOMStorageDBThread_h___

#include "prthread.h"
#include "prinrval.h"
#include "nsTArray.h"
#include "mozilla/Monitor.h"
#include "mozilla/storage/StatementCache.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsClassHashtable.h"
#include "nsIFile.h"

class mozIStorageConnection;

namespace mozilla {
namespace dom {

class DOMStorageCacheBridge;
class DOMStorageUsageBridge;
class DOMStorageUsage;

typedef mozilla::storage::StatementCache<mozIStorageStatement> StatementCache;



class DOMStorageDBBridge
{
public:
  DOMStorageDBBridge();
  virtual ~DOMStorageDBBridge() {}

  
  virtual nsresult Init() = 0;

  
  virtual nsresult Shutdown() = 0;

  
  
  
  
  virtual void AsyncPreload(DOMStorageCacheBridge* aCache, bool aPriority = false) = 0;

  
  
  virtual void AsyncGetUsage(DOMStorageUsageBridge* aUsage) = 0;

  
  
  virtual void SyncPreload(DOMStorageCacheBridge* aCache, bool aForceSync = false) = 0;

  
  virtual nsresult AsyncAddItem(DOMStorageCacheBridge* aCache, const nsAString& aKey, const nsAString& aValue) = 0;

  
  virtual nsresult AsyncUpdateItem(DOMStorageCacheBridge* aCache, const nsAString& aKey, const nsAString& aValue) = 0;

  
  virtual nsresult AsyncRemoveItem(DOMStorageCacheBridge* aCache, const nsAString& aKey) = 0;

  
  virtual nsresult AsyncClear(DOMStorageCacheBridge* aCache) = 0;

  
  virtual void AsyncClearAll() = 0;

  
  virtual void AsyncClearMatchingScope(const nsACString& aScope) = 0;

  
  virtual void AsyncFlush() = 0;

  
  virtual bool ShouldPreloadScope(const nsACString& aScope) = 0;

  
  virtual void GetScopesHavingData(InfallibleTArray<nsCString>* aScopes) = 0;
};






class DOMStorageDBThread MOZ_FINAL : public DOMStorageDBBridge
{
public:
  class PendingOperations;

  
  
  class DBOperation
  {
  public:
    typedef enum {
      
      opPreload,
      
      opPreloadUrgent,

      
      opGetUsage,

      
      opAddItem,
      opUpdateItem,
      opRemoveItem,
      opClear,

      
      opClearAll,
      opClearMatchingScope,
    } OperationType;

    explicit DBOperation(const OperationType aType,
                         DOMStorageCacheBridge* aCache = nullptr,
                         const nsAString& aKey = EmptyString(),
                         const nsAString& aValue = EmptyString());
    DBOperation(const OperationType aType,
                DOMStorageUsageBridge* aUsage);
    DBOperation(const OperationType aType,
                const nsACString& aScope);
    ~DBOperation();

    
    void PerformAndFinalize(DOMStorageDBThread* aThread);

    
    void Finalize(nsresult aRv);

    
    OperationType Type() { return mType; }

    
    const nsCString Scope();

    
    const nsCString Target();

  private:
    
    nsresult Perform(DOMStorageDBThread* aThread);

    friend class PendingOperations;
    OperationType mType;
    nsRefPtr<DOMStorageCacheBridge> mCache;
    nsRefPtr<DOMStorageUsageBridge> mUsage;
    nsString mKey;
    nsString mValue;
    nsCString mScope;
  };

  
  
  class PendingOperations {
  public:
    PendingOperations();

    
    
    void Add(DBOperation* aOperation);

    
    bool HasTasks();

    
    
    bool Prepare();

    
    
    nsresult Execute(DOMStorageDBThread* aThread);

    
    
    bool Finalize(nsresult aRv);

    
    
    bool IsScopeClearPending(const nsACString& aScope);

    
    bool IsScopeUpdatePending(const nsACString& aScope);

  private:
    
    
    bool CheckForCoalesceOpportunity(DBOperation* aNewOp,
                                     DBOperation::OperationType aPendingType,
                                     DBOperation::OperationType aNewType);

    
    nsClassHashtable<nsCStringHashKey, DBOperation> mClears;

    
    nsClassHashtable<nsCStringHashKey, DBOperation> mUpdates;

    
    nsTArray<nsAutoPtr<DBOperation> > mExecList;

    
    uint32_t mFlushFailureCount;
  };

public:
  DOMStorageDBThread();
  virtual ~DOMStorageDBThread() {}

  virtual nsresult Init();
  virtual nsresult Shutdown();

  virtual void AsyncPreload(DOMStorageCacheBridge* aCache, bool aPriority = false)
    { InsertDBOp(new DBOperation(aPriority ? DBOperation::opPreloadUrgent : DBOperation::opPreload, aCache)); }

  virtual void SyncPreload(DOMStorageCacheBridge* aCache, bool aForce = false);

  virtual void AsyncGetUsage(DOMStorageUsageBridge * aUsage)
    { InsertDBOp(new DBOperation(DBOperation::opGetUsage, aUsage)); }

  virtual nsresult AsyncAddItem(DOMStorageCacheBridge* aCache, const nsAString& aKey, const nsAString& aValue)
    { return InsertDBOp(new DBOperation(DBOperation::opAddItem, aCache, aKey, aValue)); }

  virtual nsresult AsyncUpdateItem(DOMStorageCacheBridge* aCache, const nsAString& aKey, const nsAString& aValue)
    { return InsertDBOp(new DBOperation(DBOperation::opUpdateItem, aCache, aKey, aValue)); }

  virtual nsresult AsyncRemoveItem(DOMStorageCacheBridge* aCache, const nsAString& aKey)
    { return InsertDBOp(new DBOperation(DBOperation::opRemoveItem, aCache, aKey)); }

  virtual nsresult AsyncClear(DOMStorageCacheBridge* aCache)
    { return InsertDBOp(new DBOperation(DBOperation::opClear, aCache)); }

  virtual void AsyncClearAll()
    { InsertDBOp(new DBOperation(DBOperation::opClearAll)); }

  virtual void AsyncClearMatchingScope(const nsACString& aScope)
    { InsertDBOp(new DBOperation(DBOperation::opClearMatchingScope, aScope)); }

  virtual void AsyncFlush();

  virtual bool ShouldPreloadScope(const nsACString& aScope);
  virtual void GetScopesHavingData(InfallibleTArray<nsCString>* aScopes);

private:
  nsCOMPtr<nsIFile> mDatabaseFile;
  PRThread* mThread;

  
  Monitor mMonitor;

  
  bool mStopIOThread;

  
  bool mWALModeEnabled;

  
  
  bool mDBReady;

  
  nsresult mStatus;

  
  nsTHashtable<nsCStringHashKey> mScopesHavingData;

  StatementCache mWorkerStatements;
  StatementCache mReaderStatements;

  
  nsCOMPtr<mozIStorageConnection> mWorkerConnection;

  
  nsCOMPtr<mozIStorageConnection> mReaderConnection;

  
  
  PRIntervalTime mDirtyEpoch;

  
  bool mFlushImmediately;

  
  
  nsTArray<DBOperation*> mPreloads;

  
  PendingOperations mPendingTasks;

  
  int32_t mPriorityCounter;

  
  
  nsresult InsertDBOp(DBOperation* aOperation);

  
  nsresult OpenDatabaseConnection();
  nsresult InitDatabase();
  nsresult ShutdownDatabase();

  
  nsresult SetJournalMode(bool aIsWal);
  nsresult TryJournalMode();

  
  nsresult ConfigureWALBehavior();

  void SetHigherPriority();
  void SetDefaultPriority();

  
  void ScheduleFlush();

  
  void UnscheduleFlush();

  
  
  
  
  
  
  
  
  
  
  PRIntervalTime TimeUntilFlush();

  
  void NotifyFlushCompletion();

  
  static void ThreadFunc(void* aArg);
  void ThreadFunc();
};

} 
} 

#endif 
