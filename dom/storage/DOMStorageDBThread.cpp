




#include "DOMStorageDBThread.h"
#include "DOMStorageCache.h"

#include "nsIEffectiveTLDService.h"
#include "nsDirectoryServiceUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "mozStorageCID.h"
#include "mozStorageHelper.h"
#include "mozIStorageService.h"
#include "mozIStorageBindingParamsArray.h"
#include "mozIStorageBindingParams.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageFunction.h"
#include "nsIObserverService.h"
#include "nsIVariant.h"
#include "mozilla/IOInterposer.h"
#include "mozilla/Services.h"




#define FLUSHING_INTERVAL_MS 5000


#define MAX_WAL_SIZE_BYTES 512 * 1024

namespace mozilla {
namespace dom {

DOMStorageDBBridge::DOMStorageDBBridge()
{
}


DOMStorageDBThread::DOMStorageDBThread()
: mThread(nullptr)
, mMonitor("DOMStorageThreadMonitor")
, mStopIOThread(false)
, mWALModeEnabled(false)
, mDBReady(false)
, mStatus(NS_OK)
, mWorkerStatements(mWorkerConnection)
, mReaderStatements(mReaderConnection)
, mDirtyEpoch(0)
, mFlushImmediately(false)
, mPriorityCounter(0)
{
}

nsresult
DOMStorageDBThread::Init()
{
  nsresult rv;

  
  
  
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(mDatabaseFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDatabaseFile->Append(NS_LITERAL_STRING("webappsstore.sqlite"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageService> service =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  MonitorAutoLock monitor(mMonitor);

  mThread = PR_CreateThread(PR_USER_THREAD, &DOMStorageDBThread::ThreadFunc, this,
                            PR_PRIORITY_LOW, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD,
                            262144);
  if (!mThread) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

nsresult
DOMStorageDBThread::Shutdown()
{
  if (!mThread) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_SHUTDOWN_DATABASE_MS> timer;

  {
    MonitorAutoLock monitor(mMonitor);

    
    mFlushImmediately = true;
    mStopIOThread = true;
    monitor.Notify();
  }

  PR_JoinThread(mThread);
  mThread = nullptr;

  return mStatus;
}

void
DOMStorageDBThread::SyncPreload(DOMStorageCacheBridge* aCache, bool aForceSync)
{
  if (!aForceSync && aCache->LoadedCount()) {
    
    
    SetHigherPriority();
    aCache->LoadWait();
    SetDefaultPriority();
    return;
  }

  
  
  
  if (mDBReady && mWALModeEnabled) {
    bool pendingTasks;
    {
      MonitorAutoLock monitor(mMonitor);
      pendingTasks = mPendingTasks.IsScopeUpdatePending(aCache->Scope()) ||
                     mPendingTasks.IsScopeClearPending(aCache->Scope());
    }

    if (!pendingTasks) {
      
      DBOperation preload(DBOperation::opPreload, aCache);
      preload.PerformAndFinalize(this);
      return;
    }
  }

  
  
  
  nsresult rv = InsertDBOp(new DBOperation(DBOperation::opPreloadUrgent, aCache));

  
  if (NS_SUCCEEDED(rv)) {
    aCache->LoadWait();
  }
}

void
DOMStorageDBThread::AsyncFlush()
{
  MonitorAutoLock monitor(mMonitor);
  mFlushImmediately = true;
  monitor.Notify();
}

bool
DOMStorageDBThread::ShouldPreloadScope(const nsACString& aScope)
{
  MonitorAutoLock monitor(mMonitor);
  return mScopesHavingData.Contains(aScope);
}

namespace { 

PLDHashOperator
GetScopesHavingDataEnum(nsCStringHashKey* aKey, void* aArg)
{
  InfallibleTArray<nsCString>* scopes =
      static_cast<InfallibleTArray<nsCString>*>(aArg);
  scopes->AppendElement(aKey->GetKey());
  return PL_DHASH_NEXT;
}

} 

void
DOMStorageDBThread::GetScopesHavingData(InfallibleTArray<nsCString>* aScopes)
{
  MonitorAutoLock monitor(mMonitor);
  mScopesHavingData.EnumerateEntries(GetScopesHavingDataEnum, aScopes);
}

nsresult
DOMStorageDBThread::InsertDBOp(DOMStorageDBThread::DBOperation* aOperation)
{
  MonitorAutoLock monitor(mMonitor);

  
  nsAutoPtr<DOMStorageDBThread::DBOperation> opScope(aOperation);

  if (mStopIOThread) {
    
    MOZ_ASSERT(false);
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (NS_FAILED(mStatus)) {
    MonitorAutoUnlock unlock(mMonitor);
    aOperation->Finalize(mStatus);
    return mStatus;
  }

  switch (aOperation->Type()) {
  case DBOperation::opPreload:
  case DBOperation::opPreloadUrgent:
    if (mPendingTasks.IsScopeUpdatePending(aOperation->Scope())) {
      
      
      
      
      
      mFlushImmediately = true;
    } else if (mPendingTasks.IsScopeClearPending(aOperation->Scope())) {
      
      
      
      
      
      MonitorAutoUnlock unlock(mMonitor);
      aOperation->Finalize(NS_OK);
      return NS_OK;
    }
    

  case DBOperation::opGetUsage:
    if (aOperation->Type() == DBOperation::opPreloadUrgent) {
      SetHigherPriority(); 
      mPreloads.InsertElementAt(0, aOperation);
    } else {
      mPreloads.AppendElement(aOperation);
    }

    
    opScope.forget();

    
    monitor.Notify();
    break;

  default:
    
    
    mPendingTasks.Add(aOperation);

    
    opScope.forget();

    ScheduleFlush();
    break;
  }

  return NS_OK;
}

void
DOMStorageDBThread::SetHigherPriority()
{
  ++mPriorityCounter;
  PR_SetThreadPriority(mThread, PR_PRIORITY_URGENT);
}

void
DOMStorageDBThread::SetDefaultPriority()
{
  if (--mPriorityCounter <= 0) {
    PR_SetThreadPriority(mThread, PR_PRIORITY_LOW);
  }
}

void
DOMStorageDBThread::ThreadFunc(void* aArg)
{
  PR_SetCurrentThreadName("localStorage DB");
  mozilla::IOInterposer::RegisterCurrentThread();

  DOMStorageDBThread* thread = static_cast<DOMStorageDBThread*>(aArg);
  thread->ThreadFunc();
  mozilla::IOInterposer::UnregisterCurrentThread();
}

void
DOMStorageDBThread::ThreadFunc()
{
  nsresult rv = InitDatabase();

  MonitorAutoLock lockMonitor(mMonitor);

  if (NS_FAILED(rv)) {
    mStatus = rv;
    mStopIOThread = true;
    return;
  }

  while (MOZ_LIKELY(!mStopIOThread || mPreloads.Length() || mPendingTasks.HasTasks())) {
    if (MOZ_UNLIKELY(TimeUntilFlush() == 0)) {
      
      UnscheduleFlush();
      if (mPendingTasks.Prepare()) {
        {
          MonitorAutoUnlock unlockMonitor(mMonitor);
          rv = mPendingTasks.Execute(this);
        }

        if (!mPendingTasks.Finalize(rv)) {
          mStatus = rv;
          NS_WARNING("localStorage DB access broken");
        }
      }
      NotifyFlushCompletion();
    } else if (MOZ_LIKELY(mPreloads.Length())) {
      nsAutoPtr<DBOperation> op(mPreloads[0]);
      mPreloads.RemoveElementAt(0);
      {
        MonitorAutoUnlock unlockMonitor(mMonitor);
        op->PerformAndFinalize(this);
      }

      if (op->Type() == DBOperation::opPreloadUrgent) {
        SetDefaultPriority(); 
      }
    } else if (MOZ_UNLIKELY(!mStopIOThread)) {
      lockMonitor.Wait(TimeUntilFlush());
    }
  } 

  mStatus = ShutdownDatabase();
}

extern void
ReverseString(const nsCSubstring& aSource, nsCSubstring& aResult);

namespace { 

class nsReverseStringSQLFunction MOZ_FINAL : public mozIStorageFunction
{
  ~nsReverseStringSQLFunction() {}

  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION
};

NS_IMPL_ISUPPORTS(nsReverseStringSQLFunction, mozIStorageFunction)

NS_IMETHODIMP
nsReverseStringSQLFunction::OnFunctionCall(
    mozIStorageValueArray* aFunctionArguments, nsIVariant** aResult)
{
  nsresult rv;

  nsAutoCString stringToReverse;
  rv = aFunctionArguments->GetUTF8String(0, stringToReverse);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString result;
  ReverseString(stringToReverse, result);

  nsCOMPtr<nsIWritableVariant> outVar(do_CreateInstance(
      NS_VARIANT_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = outVar->SetAsAUTF8String(result);
  NS_ENSURE_SUCCESS(rv, rv);

  *aResult = outVar.get();
  outVar.forget();
  return NS_OK;
}

} 

nsresult
DOMStorageDBThread::OpenDatabaseConnection()
{
  nsresult rv;

  MOZ_ASSERT(!NS_IsMainThread());

  nsCOMPtr<mozIStorageService> service
      = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageConnection> connection;
  rv = service->OpenUnsharedDatabase(mDatabaseFile, getter_AddRefs(mWorkerConnection));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = mDatabaseFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = service->OpenUnsharedDatabase(mDatabaseFile, getter_AddRefs(mWorkerConnection));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
DOMStorageDBThread::InitDatabase()
{
  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_INIT_DATABASE_MS> timer;

  nsresult rv;

  
  MOZ_ASSERT(!NS_IsMainThread());

  rv = OpenDatabaseConnection();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = TryJournalMode();
  NS_ENSURE_SUCCESS(rv, rv);

  
  (void)mWorkerConnection->Clone(true, getter_AddRefs(mReaderConnection));
  NS_ENSURE_TRUE(mReaderConnection, NS_ERROR_FAILURE);

  mozStorageTransaction transaction(mWorkerConnection, false);

  
  rv = mWorkerConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
         "CREATE TABLE IF NOT EXISTS webappsstore2 ("
         "scope TEXT, "
         "key TEXT, "
         "value TEXT, "
         "secure INTEGER, "
         "owner TEXT)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mWorkerConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE UNIQUE INDEX IF NOT EXISTS scope_key_index"
        " ON webappsstore2(scope, key)"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageFunction> function1(new nsReverseStringSQLFunction());
  NS_ENSURE_TRUE(function1, NS_ERROR_OUT_OF_MEMORY);

  rv = mWorkerConnection->CreateFunction(NS_LITERAL_CSTRING("REVERSESTRING"), 1, function1);
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;

  
  
  
  
  rv = mWorkerConnection->TableExists(NS_LITERAL_CSTRING("webappsstore"),
                                &exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    rv = mWorkerConnection->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("INSERT OR IGNORE INTO "
                         "webappsstore2(scope, key, value, secure, owner) "
                         "SELECT REVERSESTRING(domain) || '.:', key, value, secure, owner "
                         "FROM webappsstore"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mWorkerConnection->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("DROP TABLE webappsstore"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  rv = mWorkerConnection->TableExists(NS_LITERAL_CSTRING("moz_webappsstore"),
                                &exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    rv = mWorkerConnection->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("INSERT OR IGNORE INTO "
                         "webappsstore2(scope, key, value, secure, owner) "
                         "SELECT REVERSESTRING(domain) || '.:', key, value, secure, domain "
                         "FROM moz_webappsstore"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mWorkerConnection->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("DROP TABLE moz_webappsstore"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  mDBReady = true;

  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mWorkerConnection->CreateStatement(NS_LITERAL_CSTRING("SELECT DISTINCT scope FROM webappsstore2"),
                                    getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper scope(stmt);

  while (NS_SUCCEEDED(rv = stmt->ExecuteStep(&exists)) && exists) {
    nsAutoCString foundScope;
    rv = stmt->GetUTF8String(0, foundScope);
    NS_ENSURE_SUCCESS(rv, rv);

    MonitorAutoLock monitor(mMonitor);
    mScopesHavingData.PutEntry(foundScope);
  }

  return NS_OK;
}

nsresult
DOMStorageDBThread::SetJournalMode(bool aIsWal)
{
  nsresult rv;

  nsAutoCString stmtString(
    MOZ_STORAGE_UNIQUIFY_QUERY_STR "PRAGMA journal_mode = ");
  if (aIsWal) {
    stmtString.AppendLiteral("wal");
  } else {
    stmtString.AppendLiteral("truncate");
  }

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mWorkerConnection->CreateStatement(stmtString, getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper scope(stmt);

  bool hasResult = false;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasResult) {
    return NS_ERROR_FAILURE;
  }

  nsAutoCString journalMode;
  rv = stmt->GetUTF8String(0, journalMode);
  NS_ENSURE_SUCCESS(rv, rv);
  if ((aIsWal && !journalMode.EqualsLiteral("wal")) ||
      (!aIsWal && !journalMode.EqualsLiteral("truncate"))) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
DOMStorageDBThread::TryJournalMode()
{
  nsresult rv;

  rv = SetJournalMode(true);
  if (NS_FAILED(rv)) {
    mWALModeEnabled = false;

    rv = SetJournalMode(false);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    mWALModeEnabled = true;

    rv = ConfigureWALBehavior();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
DOMStorageDBThread::ConfigureWALBehavior()
{
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mWorkerConnection->CreateStatement(NS_LITERAL_CSTRING(
    MOZ_STORAGE_UNIQUIFY_QUERY_STR "PRAGMA page_size"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResult = false;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && hasResult, NS_ERROR_FAILURE);

  int32_t pageSize = 0;
  rv = stmt->GetInt32(0, &pageSize);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && pageSize > 0, NS_ERROR_UNEXPECTED);

  
  
  int32_t thresholdInPages = static_cast<int32_t>(MAX_WAL_SIZE_BYTES / pageSize);
  nsAutoCString thresholdPragma("PRAGMA wal_autocheckpoint = ");
  thresholdPragma.AppendInt(thresholdInPages);
  rv = mWorkerConnection->ExecuteSimpleSQL(thresholdPragma);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsAutoCString journalSizePragma("PRAGMA journal_size_limit = ");
  
  journalSizePragma.AppendInt(MAX_WAL_SIZE_BYTES * 3);
  rv = mWorkerConnection->ExecuteSimpleSQL(journalSizePragma);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
DOMStorageDBThread::ShutdownDatabase()
{
  
  MOZ_ASSERT(!NS_IsMainThread());

  nsresult rv = mStatus;

  mDBReady = false;

  
  mReaderStatements.FinalizeStatements();
  mWorkerStatements.FinalizeStatements();

  if (mReaderConnection) {
    
    
    mReaderConnection->Close();
    mReaderConnection = nullptr;
  }

  if (mWorkerConnection) {
    rv = mWorkerConnection->Close();
    mWorkerConnection = nullptr;
  }

  return rv;
}

void
DOMStorageDBThread::ScheduleFlush()
{
  if (mDirtyEpoch) {
    return; 
  }

  mDirtyEpoch = PR_IntervalNow() | 1; 

  
  mMonitor.Notify();
}

void
DOMStorageDBThread::UnscheduleFlush()
{
  
  mFlushImmediately = false;
  mDirtyEpoch = 0;
}

PRIntervalTime
DOMStorageDBThread::TimeUntilFlush()
{
  if (mFlushImmediately) {
    return 0; 
  }

  static_assert(PR_INTERVAL_NO_TIMEOUT != 0,
      "PR_INTERVAL_NO_TIMEOUT must be non-zero");

  if (!mDirtyEpoch) {
    return PR_INTERVAL_NO_TIMEOUT; 
  }

  static const PRIntervalTime kMaxAge = PR_MillisecondsToInterval(FLUSHING_INTERVAL_MS);

  PRIntervalTime now = PR_IntervalNow() | 1;
  PRIntervalTime age = now - mDirtyEpoch;
  if (age > kMaxAge) {
    return 0; 
  }

  return kMaxAge - age; 
}

void
DOMStorageDBThread::NotifyFlushCompletion()
{
#ifdef DOM_STORAGE_TESTS
  if (!NS_IsMainThread()) {
    nsRefPtr<nsRunnableMethod<DOMStorageDBThread, void, false> > event =
      NS_NewNonOwningRunnableMethod(this, &DOMStorageDBThread::NotifyFlushCompletion);
    NS_DispatchToMainThread(event);
    return;
  }

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(nullptr, "domstorage-test-flushed", nullptr);
  }
#endif
}



DOMStorageDBThread::DBOperation::DBOperation(const OperationType aType,
                                             DOMStorageCacheBridge* aCache,
                                             const nsAString& aKey,
                                             const nsAString& aValue)
: mType(aType)
, mCache(aCache)
, mKey(aKey)
, mValue(aValue)
{
  MOZ_COUNT_CTOR(DOMStorageDBThread::DBOperation);
}

DOMStorageDBThread::DBOperation::DBOperation(const OperationType aType,
                                             DOMStorageUsageBridge* aUsage)
: mType(aType)
, mUsage(aUsage)
{
  MOZ_COUNT_CTOR(DOMStorageDBThread::DBOperation);
}

DOMStorageDBThread::DBOperation::DBOperation(const OperationType aType,
                                             const nsACString& aScope)
: mType(aType)
, mCache(nullptr)
, mScope(aScope)
{
  MOZ_COUNT_CTOR(DOMStorageDBThread::DBOperation);
}

DOMStorageDBThread::DBOperation::~DBOperation()
{
  MOZ_COUNT_DTOR(DOMStorageDBThread::DBOperation);
}

const nsCString
DOMStorageDBThread::DBOperation::Scope()
{
  if (mCache) {
    return mCache->Scope();
  }

  return mScope;
}

const nsCString
DOMStorageDBThread::DBOperation::Target()
{
  switch (mType) {
    case opAddItem:
    case opUpdateItem:
    case opRemoveItem:
      return Scope() + NS_LITERAL_CSTRING("|") + NS_ConvertUTF16toUTF8(mKey);

    default:
      return Scope();
  }
}

void
DOMStorageDBThread::DBOperation::PerformAndFinalize(DOMStorageDBThread* aThread)
{
  Finalize(Perform(aThread));
}

nsresult
DOMStorageDBThread::DBOperation::Perform(DOMStorageDBThread* aThread)
{
  nsresult rv;

  switch (mType) {
  case opPreload:
  case opPreloadUrgent:
  {
    
    if (mCache->Loaded()) {
      break;
    }

    StatementCache* statements;
    if (MOZ_UNLIKELY(NS_IsMainThread())) {
      statements = &aThread->mReaderStatements;
    } else {
      statements = &aThread->mWorkerStatements;
    }

    
    
    
    nsCOMPtr<mozIStorageStatement> stmt = statements->GetCachedStatement(
        "SELECT key, value FROM webappsstore2 "
        "WHERE scope = :scope ORDER BY key "
        "LIMIT -1 OFFSET :offset");
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scope(stmt);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                    mCache->Scope());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("offset"),
                               static_cast<int32_t>(mCache->LoadedCount()));
    NS_ENSURE_SUCCESS(rv, rv);

    bool exists;
    while (NS_SUCCEEDED(rv = stmt->ExecuteStep(&exists)) && exists) {
      nsAutoString key;
      rv = stmt->GetString(0, key);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString value;
      rv = stmt->GetString(1, value);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!mCache->LoadItem(key, value)) {
        break;
      }
    }

    mCache->LoadDone(NS_OK);
    break;
  }

  case opGetUsage:
  {
    nsCOMPtr<mozIStorageStatement> stmt = aThread->mWorkerStatements.GetCachedStatement(
      "SELECT SUM(LENGTH(key) + LENGTH(value)) FROM webappsstore2"
      " WHERE scope LIKE :scope"
    );
    NS_ENSURE_STATE(stmt);

    mozStorageStatementScoper scope(stmt);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                    mUsage->Scope() + NS_LITERAL_CSTRING("%"));
    NS_ENSURE_SUCCESS(rv, rv);

    bool exists;
    rv = stmt->ExecuteStep(&exists);
    NS_ENSURE_SUCCESS(rv, rv);

    int64_t usage = 0;
    if (exists) {
      rv = stmt->GetInt64(0, &usage);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mUsage->LoadUsage(usage);
    break;
  }

  case opAddItem:
  case opUpdateItem:
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsCOMPtr<mozIStorageStatement> stmt = aThread->mWorkerStatements.GetCachedStatement(
      "INSERT OR REPLACE INTO webappsstore2 (scope, key, value) "
      "VALUES (:scope, :key, :value) "
    );
    NS_ENSURE_STATE(stmt);

    mozStorageStatementScoper scope(stmt);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                    mCache->Scope());
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("key"),
                                mKey);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("value"),
                                mValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    aThread->mScopesHavingData.PutEntry(Scope());
    break;
  }

  case opRemoveItem:
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsCOMPtr<mozIStorageStatement> stmt = aThread->mWorkerStatements.GetCachedStatement(
      "DELETE FROM webappsstore2 "
      "WHERE scope = :scope "
        "AND key = :key "
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scope(stmt);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                    mCache->Scope());
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("key"),
                                mKey);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    break;
  }

  case opClear:
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsCOMPtr<mozIStorageStatement> stmt = aThread->mWorkerStatements.GetCachedStatement(
      "DELETE FROM webappsstore2 "
      "WHERE scope = :scope"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scope(stmt);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                    mCache->Scope());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    aThread->mScopesHavingData.RemoveEntry(Scope());
    break;
  }

  case opClearAll:
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsCOMPtr<mozIStorageStatement> stmt = aThread->mWorkerStatements.GetCachedStatement(
      "DELETE FROM webappsstore2"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scope(stmt);

    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    aThread->mScopesHavingData.Clear();
    break;
  }

  case opClearMatchingScope:
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsCOMPtr<mozIStorageStatement> stmt = aThread->mWorkerStatements.GetCachedStatement(
      "DELETE FROM webappsstore2"
      " WHERE scope GLOB :scope"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scope(stmt);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                    mScope + NS_LITERAL_CSTRING("*"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    break;
  }

  default:
    NS_ERROR("Unknown task type");
    break;
  }

  return NS_OK;
}

void
DOMStorageDBThread::DBOperation::Finalize(nsresult aRv)
{
  switch (mType) {
  case opPreloadUrgent:
  case opPreload:
    if (NS_FAILED(aRv)) {
      
      
      
      NS_WARNING("Failed to preload localStorage");
    }

    mCache->LoadDone(aRv);
    break;

  case opGetUsage:
    if (NS_FAILED(aRv)) {
      mUsage->LoadUsage(0);
    }

    break;

  default:
    if (NS_FAILED(aRv)) {
      NS_WARNING("localStorage update/clear operation failed,"
                 " data may not persist or clean up");
    }

    break;
  }
}



DOMStorageDBThread::PendingOperations::PendingOperations()
: mFlushFailureCount(0)
{
}

bool
DOMStorageDBThread::PendingOperations::HasTasks()
{
  return !!mUpdates.Count() || !!mClears.Count();
}

namespace { 

PLDHashOperator
ForgetUpdatesForScope(const nsACString& aMapping,
                      nsAutoPtr<DOMStorageDBThread::DBOperation>& aPendingTask,
                      void* aArg)
{
  DOMStorageDBThread::DBOperation* newOp = static_cast<DOMStorageDBThread::DBOperation*>(aArg);

  if (newOp->Type() == DOMStorageDBThread::DBOperation::opClear &&
      aPendingTask->Scope() != newOp->Scope()) {
    return PL_DHASH_NEXT;
  }

  if (newOp->Type() == DOMStorageDBThread::DBOperation::opClearMatchingScope &&
      !StringBeginsWith(aPendingTask->Scope(), newOp->Scope())) {
    return PL_DHASH_NEXT;
  }

  return PL_DHASH_REMOVE;
}

} 

bool
DOMStorageDBThread::PendingOperations::CheckForCoalesceOpportunity(DBOperation* aNewOp,
                                                                   DBOperation::OperationType aPendingType,
                                                                   DBOperation::OperationType aNewType)
{
  if (aNewOp->Type() != aNewType) {
    return false;
  }

  DOMStorageDBThread::DBOperation* pendingTask;
  if (!mUpdates.Get(aNewOp->Target(), &pendingTask)) {
    return false;
  }

  if (pendingTask->Type() != aPendingType) {
    return false;
  }

  return true;
}

void
DOMStorageDBThread::PendingOperations::Add(DOMStorageDBThread::DBOperation* aOperation)
{
  
  
  
  if (CheckForCoalesceOpportunity(aOperation, DBOperation::opAddItem, DBOperation::opRemoveItem)) {
    mUpdates.Remove(aOperation->Target());
    delete aOperation;
    return;
  }

  
  
  
  
  if (CheckForCoalesceOpportunity(aOperation, DBOperation::opAddItem, DBOperation::opUpdateItem)) {
    aOperation->mType = DBOperation::opAddItem;
  }

  
  
  
  
  if (CheckForCoalesceOpportunity(aOperation, DBOperation::opRemoveItem, DBOperation::opAddItem)) {
    aOperation->mType = DBOperation::opUpdateItem;
  }

  switch (aOperation->Type())
  {
  

  case DBOperation::opAddItem:
  case DBOperation::opUpdateItem:
  case DBOperation::opRemoveItem:
    
    mUpdates.Put(aOperation->Target(), aOperation);
    break;

  

  case DBOperation::opClear:
  case DBOperation::opClearMatchingScope:
    
    
    
    
    mUpdates.Enumerate(ForgetUpdatesForScope, aOperation);
    mClears.Put(aOperation->Target(), aOperation);
    break;

  case DBOperation::opClearAll:
    
    mUpdates.Clear();
    mClears.Clear();
    mClears.Put(aOperation->Target(), aOperation);
    break;

  default:
    MOZ_ASSERT(false);
    break;
  }
}

namespace { 

PLDHashOperator
CollectTasks(const nsACString& aMapping, nsAutoPtr<DOMStorageDBThread::DBOperation>& aOperation, void* aArg)
{
  nsTArray<nsAutoPtr<DOMStorageDBThread::DBOperation> >* tasks =
    static_cast<nsTArray<nsAutoPtr<DOMStorageDBThread::DBOperation> >*>(aArg);

  tasks->AppendElement(aOperation.forget());
  return PL_DHASH_NEXT;
}

} 

bool
DOMStorageDBThread::PendingOperations::Prepare()
{
  

  
  
  
  
  
  mClears.Enumerate(CollectTasks, &mExecList);
  mClears.Clear();

  mUpdates.Enumerate(CollectTasks, &mExecList);
  mUpdates.Clear();

  return !!mExecList.Length();
}

nsresult
DOMStorageDBThread::PendingOperations::Execute(DOMStorageDBThread* aThread)
{
  

  mozStorageTransaction transaction(aThread->mWorkerConnection, false);

  nsresult rv;

  for (uint32_t i = 0; i < mExecList.Length(); ++i) {
    DOMStorageDBThread::DBOperation* task = mExecList[i];
    rv = task->Perform(aThread);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  rv = transaction.Commit();
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

bool
DOMStorageDBThread::PendingOperations::Finalize(nsresult aRv)
{
  

  
  if (NS_FAILED(aRv)) {
    
    
    
    
    
    
    NS_WARNING("Flush operation on localStorage database failed");

    ++mFlushFailureCount;

    return mFlushFailureCount >= 5;
  }

  mFlushFailureCount = 0;
  mExecList.Clear();
  return true;
}

namespace { 

class FindPendingOperationForScopeData
{
public:
  explicit FindPendingOperationForScopeData(const nsACString& aScope) : mScope(aScope), mFound(false) {}
  nsCString mScope;
  bool mFound;
};

PLDHashOperator
FindPendingClearForScope(const nsACString& aMapping,
                         DOMStorageDBThread::DBOperation* aPendingOperation,
                         void* aArg)
{
  FindPendingOperationForScopeData* data =
    static_cast<FindPendingOperationForScopeData*>(aArg);

  if (aPendingOperation->Type() == DOMStorageDBThread::DBOperation::opClearAll) {
    data->mFound = true;
    return PL_DHASH_STOP;
  }

  if (aPendingOperation->Type() == DOMStorageDBThread::DBOperation::opClear &&
      data->mScope == aPendingOperation->Scope()) {
    data->mFound = true;
    return PL_DHASH_STOP;
  }

  if (aPendingOperation->Type() == DOMStorageDBThread::DBOperation::opClearMatchingScope &&
      StringBeginsWith(data->mScope, aPendingOperation->Scope())) {
    data->mFound = true;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

} 

bool
DOMStorageDBThread::PendingOperations::IsScopeClearPending(const nsACString& aScope)
{
  

  FindPendingOperationForScopeData data(aScope);
  mClears.EnumerateRead(FindPendingClearForScope, &data);
  if (data.mFound) {
    return true;
  }

  for (uint32_t i = 0; i < mExecList.Length(); ++i) {
    DOMStorageDBThread::DBOperation* task = mExecList[i];
    FindPendingClearForScope(EmptyCString(), task, &data);

    if (data.mFound) {
      return true;
    }
  }

  return false;
}

namespace { 

PLDHashOperator
FindPendingUpdateForScope(const nsACString& aMapping,
                          DOMStorageDBThread::DBOperation* aPendingOperation,
                          void* aArg)
{
  FindPendingOperationForScopeData* data =
    static_cast<FindPendingOperationForScopeData*>(aArg);

  if ((aPendingOperation->Type() == DOMStorageDBThread::DBOperation::opAddItem ||
       aPendingOperation->Type() == DOMStorageDBThread::DBOperation::opUpdateItem ||
       aPendingOperation->Type() == DOMStorageDBThread::DBOperation::opRemoveItem) &&
       data->mScope == aPendingOperation->Scope()) {
    data->mFound = true;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

} 

bool
DOMStorageDBThread::PendingOperations::IsScopeUpdatePending(const nsACString& aScope)
{
  

  FindPendingOperationForScopeData data(aScope);
  mUpdates.EnumerateRead(FindPendingUpdateForScope, &data);
  if (data.mFound) {
    return true;
  }

  for (uint32_t i = 0; i < mExecList.Length(); ++i) {
    DOMStorageDBThread::DBOperation* task = mExecList[i];
    FindPendingUpdateForScope(EmptyCString(), task, &data);

    if (data.mFound) {
      return true;
    }
  }

  return false;
}

} 
} 
