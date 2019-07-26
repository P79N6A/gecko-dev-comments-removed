




#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "nsError.h"
#include "nsDOMStorage.h"
#include "nsDOMStorageDBWrapper.h"
#include "nsDOMStoragePersistentDB.h"
#include "nsIFile.h"
#include "nsIVariant.h"
#include "nsAppDirectoryServiceDefs.h"
#include "mozStorageCID.h"
#include "mozStorageHelper.h"
#include "mozIStorageService.h"
#include "mozIStorageBindingParamsArray.h"
#include "mozIStorageBindingParams.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageFunction.h"
#include "nsNetUtil.h"
#include "mozilla/Telemetry.h"
#include "nsThreadUtils.h"

#include "sampler.h"

using namespace mozilla;


#define MAX_WAL_SIZE_BYTES 512 * 1024

namespace {




class FlushCallbackTask : public nsRunnable
{
public:
  FlushCallbackTask(nsDOMStoragePersistentDB* aDB)
    : mDB(aDB), mSuccess(true)
  {}
  NS_IMETHOD Run();
  void SetFlushResult(bool aSucceeded)
  {
    MOZ_ASSERT(!NS_IsMainThread());
    mSuccess = aSucceeded;
  }
private:
  nsDOMStoragePersistentDB* mDB;
  bool mSuccess;
};




class FlushTask : public nsRunnable {
public:
  FlushTask(nsDOMStoragePersistentDB* aDB, FlushCallbackTask* aCallback)
    : mDB(aDB), mCallback(aCallback)
  {}
  NS_IMETHOD Run();
private:
  nsDOMStoragePersistentDB* mDB;
  nsRefPtr<FlushCallbackTask> mCallback;
};





nsresult
FlushTask::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsresult rv = mDB->Flush();
  mCallback->SetFlushResult(NS_SUCCEEDED(rv));

  
  rv = NS_DispatchToMainThread(mCallback, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





nsresult
FlushCallbackTask::Run()
{
  MOZ_ASSERT(NS_IsMainThread());
  mDB->HandleFlushComplete(mSuccess);
  return NS_OK;
}

} 

nsresult
nsDOMStoragePersistentDB::Flush()
{
  if (!mFlushStatements.Length()) {
    return NS_OK;
  }

  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_TIMER_FLUSH_MS> timer;

  
  mozStorageTransaction transaction(mWriteConnection, false);

  nsresult rv;
  for (uint32_t i = 0; i < mFlushStatements.Length(); ++i) {
    if (mFlushStatementParams[i]) {
      rv = mFlushStatements[i]->BindParameters(mFlushStatementParams[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = mFlushStatements[i]->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





void
nsDOMStoragePersistentDB::HandleFlushComplete(bool aSucceeded)
{
  MOZ_ASSERT(mIsFlushPending);

  if (aSucceeded) {
    mCache.MarkScopesFlushed();
  } else {
    mCache.MarkFlushFailed();
    mWasRemoveAllCalled |= mIsRemoveAllPending;
  }

  mIsRemoveAllPending = false;
  mIsFlushPending = false;
  mFlushStatements.Clear();
  mFlushStatementParams.Clear();
}

class nsReverseStringSQLFunction MOZ_FINAL : public mozIStorageFunction
{
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION
};

NS_IMPL_ISUPPORTS1(nsReverseStringSQLFunction, mozIStorageFunction)

NS_IMETHODIMP
nsReverseStringSQLFunction::OnFunctionCall(
    mozIStorageValueArray *aFunctionArguments, nsIVariant **aResult)
{
  nsAutoCString stringToReverse;
  nsresult rv = aFunctionArguments->GetUTF8String(0, stringToReverse);
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

nsDOMStoragePersistentDB::nsDOMStoragePersistentDB()
: mReadStatements(mReadConnection), mWriteStatements(mWriteConnection),
  mWasRemoveAllCalled(false), mIsRemoveAllPending(false), mIsFlushPending(false)
{
  mQuotaUseByUncached.Init(16);
}

nsresult
nsDOMStoragePersistentDB::Init(const nsString& aDatabaseName)
{
  nsCOMPtr<nsIFile> storageFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(storageFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = storageFile->Append(aDatabaseName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageService> service;

  service = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = service->OpenUnsharedDatabase(storageFile, getter_AddRefs(mWriteConnection));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = storageFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = service->OpenUnsharedDatabase(storageFile, getter_AddRefs(mWriteConnection));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  if (NS_SUCCEEDED(SetJournalMode(true))) {
    
    rv = ::NS_NewNamedThread("DOM Storage", getter_AddRefs(mFlushThread));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = ConfigureWalBehavior();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    
    rv = SetJournalMode(false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  (void)mWriteConnection->Clone(true, getter_AddRefs(mReadConnection));
  NS_ENSURE_TRUE(mReadConnection, NS_ERROR_FAILURE);

  mozStorageTransaction transaction(mWriteConnection, false);

  
  rv = mWriteConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
         "CREATE TABLE IF NOT EXISTS webappsstore2 ("
         "scope TEXT, "
         "key TEXT, "
         "value TEXT, "
         "secure INTEGER, "
         "owner TEXT)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mWriteConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE UNIQUE INDEX IF NOT EXISTS scope_key_index"
        " ON webappsstore2(scope, key)"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageFunction> function1(new nsReverseStringSQLFunction());
  NS_ENSURE_TRUE(function1, NS_ERROR_OUT_OF_MEMORY);

  rv = mWriteConnection->CreateFunction(NS_LITERAL_CSTRING("REVERSESTRING"), 1, function1);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  bool exists;
  rv = mWriteConnection->TableExists(NS_LITERAL_CSTRING("webappsstore"),
                                     &exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
      rv = mWriteConnection->ExecuteSimpleSQL(
             NS_LITERAL_CSTRING("INSERT OR IGNORE INTO "
                                "webappsstore2(scope, key, value, secure, owner) "
                                "SELECT REVERSESTRING(domain) || '.:', key, value, secure, owner "
                                "FROM webappsstore"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mWriteConnection->ExecuteSimpleSQL(
             NS_LITERAL_CSTRING("DROP TABLE webappsstore"));
      NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  rv = mWriteConnection->TableExists(NS_LITERAL_CSTRING("moz_webappsstore"),
                                     &exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
      rv = mWriteConnection->ExecuteSimpleSQL(
             NS_LITERAL_CSTRING("INSERT OR IGNORE INTO "
                                "webappsstore2(scope, key, value, secure, owner) "
                                "SELECT REVERSESTRING(domain) || '.:', key, value, secure, domain "
                                "FROM moz_webappsstore"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mWriteConnection->ExecuteSimpleSQL(
             NS_LITERAL_CSTRING("DROP TABLE moz_webappsstore"));
      NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::SetJournalMode(bool aIsWal)
{
  nsAutoCString stmtString(
    MOZ_STORAGE_UNIQUIFY_QUERY_STR "PRAGMA journal_mode = ");
  if (aIsWal) {
    stmtString.AppendLiteral("wal");
  } else {
    stmtString.AppendLiteral("truncate");
  }

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mWriteConnection->CreateStatement(stmtString, getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

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
nsDOMStoragePersistentDB::ConfigureWalBehavior()
{
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mWriteConnection->CreateStatement(NS_LITERAL_CSTRING(
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
  rv = mWriteConnection->ExecuteSimpleSQL(thresholdPragma);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsAutoCString journalSizePragma("PRAGMA journal_size_limit = ");
  
  journalSizePragma.AppendInt(MAX_WAL_SIZE_BYTES * 3);
  rv = mWriteConnection->ExecuteSimpleSQL(journalSizePragma);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsDOMStoragePersistentDB::Close()
{
  if (mFlushThread) {
    mFlushThread->Shutdown();
    mFlushThread = nullptr;
  }

  mFlushStatements.Clear();
  mFlushStatementParams.Clear();
  mReadStatements.FinalizeStatements();
  mWriteStatements.FinalizeStatements();
  mReadConnection->Close();
  mWriteConnection->Close();

  
  DOMStorageImpl::gStorageDB->StopCacheFlushTimer();
}

bool
nsDOMStoragePersistentDB::IsFlushTimerNeeded() const
{
  
  
  return (mCache.Count() > 0 ||
          mWasRemoveAllCalled || mIsRemoveAllPending ||
          mIsFlushPending);
}

nsresult
nsDOMStoragePersistentDB::FetchScope(DOMStorageImpl* aStorage,
                                     nsScopeCache* aScopeCache)
{
  if (mWasRemoveAllCalled || mIsRemoveAllPending) {
    return NS_OK;
  }

  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_FETCH_DOMAIN_MS> timer;

  
  nsCOMPtr<mozIStorageStatement> keysStmt = mReadStatements.GetCachedStatement(
    "SELECT key, value, secure FROM webappsstore2"
    " WHERE scope = :scopeKey"
  );
  NS_ENSURE_STATE(keysStmt);
  mozStorageStatementScoper scope(keysStmt);

  nsresult rv = keysStmt->BindUTF8StringByName(NS_LITERAL_CSTRING("scopeKey"),
                                               aStorage->GetScopeDBKey());
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  while (NS_SUCCEEDED(rv = keysStmt->ExecuteStep(&exists)) && exists) {
    nsAutoString key;
    rv = keysStmt->GetString(0, key);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString value;
    rv = keysStmt->GetString(1, value);
    NS_ENSURE_SUCCESS(rv, rv);

    int32_t secureInt = 0;
    rv = keysStmt->GetInt32(2, &secureInt);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aScopeCache->AddEntry(key, value, !!secureInt);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::EnsureScopeLoaded(DOMStorageImpl* aStorage)
{
  nsScopeCache* scopeCache = mCache.GetScope(aStorage->GetScopeDBKey());
  if (scopeCache) {
    return NS_OK;
  }

  scopeCache = new nsScopeCache();
  nsresult rv = FetchScope(aStorage, scopeCache);
  if (NS_FAILED(rv)) {
    delete scopeCache;
    return rv;
  }
  mCache.AddScope(aStorage->GetScopeDBKey(), scopeCache);

  
  nsAutoCString quotaKey(aStorage->GetQuotaDBKey());
  int32_t uncachedSizeOnDisk = 0;
  if (!quotaKey.IsEmpty() &&
      mQuotaUseByUncached.Get(quotaKey, &uncachedSizeOnDisk)) {
    uncachedSizeOnDisk -= scopeCache->GetQuotaUsage();
    MOZ_ASSERT(uncachedSizeOnDisk >= 0);
    mQuotaUseByUncached.Put(quotaKey, uncachedSizeOnDisk);
  }

  DOMStorageImpl::gStorageDB->EnsureCacheFlushTimer();

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::EnsureQuotaUsageLoaded(const nsACString& aQuotaKey)
{
  if (aQuotaKey.IsEmpty() || mQuotaUseByUncached.Get(aQuotaKey, nullptr)) {
    return NS_OK;
  } else if (mWasRemoveAllCalled || mIsRemoveAllPending) {
    mQuotaUseByUncached.Put(aQuotaKey, 0);
    return NS_OK;
  }

  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_FETCH_QUOTA_USE_MS> timer;

  
  nsCOMPtr<mozIStorageStatement> stmt;
  stmt = mReadStatements.GetCachedStatement(
    "SELECT scope, SUM(LENGTH(key) + LENGTH(value)) "
    "FROM ( "
      "SELECT scope, key, value FROM webappsstore2 "
      "WHERE scope LIKE :quotaKey"
    ") "
    "GROUP BY scope"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scope(stmt);

  nsresult rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("quotaKey"),
                                           aQuotaKey + NS_LITERAL_CSTRING("%"));
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t uncachedSize = 0;
  bool exists;
  while (NS_SUCCEEDED(rv = stmt->ExecuteStep(&exists)) && exists) {
    nsAutoCString scopeName;
    rv = stmt->GetUTF8String(0, scopeName);
    NS_ENSURE_SUCCESS(rv, rv);

    int32_t quotaUsage;
    rv = stmt->GetInt32(1, &quotaUsage);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mCache.IsScopeCached(scopeName)) {
      uncachedSize += quotaUsage;
    }
  }
  mQuotaUseByUncached.Put(aQuotaKey, uncachedSize);
  return NS_OK;
}

struct UpdateQuotaEnumData
{
  nsCString& mEvictedScope;
  int32_t mEvictedSize;
};

static PLDHashOperator
UpdateQuotaEnum(const nsACString& aQuotaKey,
                int32_t& aUncachedSize,
                void* aParams)
{
  UpdateQuotaEnumData* data = static_cast<UpdateQuotaEnumData*>(aParams);
  if (StringBeginsWith(data->mEvictedScope, aQuotaKey)) {
    aUncachedSize += data->mEvictedSize;
    return PL_DHASH_STOP;
  }
  return PL_DHASH_NEXT;
}

void
nsDOMStoragePersistentDB::EvictUnusedScopes()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsTArray<nsCString> evictedScopes;
  nsTArray<int32_t> evictedSize;
  mCache.EvictScopes(evictedScopes, evictedSize);

  
  for (uint32_t i = 0; i < evictedScopes.Length(); ++i) {
    UpdateQuotaEnumData data = { evictedScopes[i], evictedSize[i] };
    mQuotaUseByUncached.Enumerate(UpdateQuotaEnum, &data);
  }
}




namespace {

nsresult
BindScope(mozIStorageStatement* aStmt,
          const nsACString& aScopeName,
          mozIStorageBindingParamsArray** aParamArray)
{
  nsCOMPtr<mozIStorageBindingParamsArray> paramArray;
  aStmt->NewBindingParamsArray(getter_AddRefs(paramArray));

  nsCOMPtr<mozIStorageBindingParams> params;
  paramArray->NewBindingParams(getter_AddRefs(params));

  nsresult rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                             aScopeName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = paramArray->AddParams(params);
  NS_ENSURE_SUCCESS(rv, rv);

  paramArray.forget(aParamArray);

  return NS_OK;
}

nsresult
BindScopeAndKey(mozIStorageStatement* aStmt,
                const nsACString& aScopeName,
                const nsAString& aKey,
                mozIStorageBindingParamsArray** aParamArray)
{
  nsCOMPtr<mozIStorageBindingParamsArray> paramArray;
  aStmt->NewBindingParamsArray(getter_AddRefs(paramArray));

  nsCOMPtr<mozIStorageBindingParams> params;
  paramArray->NewBindingParams(getter_AddRefs(params));

  nsresult rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                             aScopeName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = params->BindStringByName(NS_LITERAL_CSTRING("key"),
                                aKey);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = paramArray->AddParams(params);
  NS_ENSURE_SUCCESS(rv, rv);

  paramArray.forget(aParamArray);

  return NS_OK;
}

nsresult
BindInsertKey(mozIStorageStatement* aStmt,
              const nsACString& aScopeName,
              const nsAString& aKey,
              const nsScopeCache::KeyEntry* aEntry,
              mozIStorageBindingParamsArray** aParamArray)
{
  nsCOMPtr<mozIStorageBindingParamsArray> paramArray;
  aStmt->NewBindingParamsArray(getter_AddRefs(paramArray));

  nsCOMPtr<mozIStorageBindingParams> params;
  paramArray->NewBindingParams(getter_AddRefs(params));

  nsresult rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                             aScopeName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = params->BindStringByName(NS_LITERAL_CSTRING("key"),
                                aKey);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = params->BindStringByName(NS_LITERAL_CSTRING("value"),
                                aEntry->mValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = params->BindInt32ByName(NS_LITERAL_CSTRING("secure"),
                               aEntry->mIsSecure ? 1 : 0);

  rv = paramArray->AddParams(params);
  NS_ENSURE_SUCCESS(rv, rv);

  paramArray.forget(aParamArray);

  return NS_OK;
}

} 

nsresult
nsDOMStoragePersistentDB::PrepareFlushStatements(const FlushData& aDirtyData)
{
  MOZ_ASSERT(NS_IsMainThread());

  mFlushStatements.Clear();
  mFlushStatementParams.Clear();

  nsCOMPtr<mozIStorageStatement> removeAllStmt = mWriteStatements.GetCachedStatement(
    NS_LITERAL_CSTRING("DELETE FROM webappsstore2"));
  NS_ENSURE_STATE(removeAllStmt);

  nsCOMPtr<mozIStorageStatement> deleteScopeStmt = mWriteStatements.GetCachedStatement(
    NS_LITERAL_CSTRING("DELETE FROM webappsstore2"
                       " WHERE scope = :scope"));
  NS_ENSURE_STATE(deleteScopeStmt);

  nsCOMPtr<mozIStorageStatement> deleteKeyStmt = mWriteStatements.GetCachedStatement(
    NS_LITERAL_CSTRING("DELETE FROM webappsstore2"
                       " WHERE scope = :scope AND key = :key"));
  NS_ENSURE_STATE(deleteKeyStmt);

  nsCOMPtr<mozIStorageStatement> insertKeyStmt = mWriteStatements.GetCachedStatement(
    NS_LITERAL_CSTRING("INSERT OR REPLACE INTO webappsstore2 (scope, key, value, secure)"
                       " VALUES (:scope, :key, :value, :secure)"));
  NS_ENSURE_STATE(insertKeyStmt);

  nsCOMPtr<mozIStorageBindingParamsArray> tempArray;
  const size_t dirtyScopeCount = aDirtyData.mScopeNames.Length();

  
  if (mWasRemoveAllCalled) {
    mFlushStatements.AppendElement(removeAllStmt);
    tempArray = nullptr;
    mFlushStatementParams.AppendElement(tempArray);
  }

  
  if (!mWasRemoveAllCalled) {
    
    nsresult rv;
    for (uint32_t i = 0; i < dirtyScopeCount; ++i) {
      if (aDirtyData.mChanged[i].mWasDeleted) {
        rv = BindScope(deleteScopeStmt,
                       aDirtyData.mScopeNames[i],
                       getter_AddRefs(tempArray));
        NS_ENSURE_SUCCESS(rv, rv);
        mFlushStatements.AppendElement(deleteScopeStmt);
        mFlushStatementParams.AppendElement(tempArray);
      }
    }

    
    for (uint32_t i = 0; i < dirtyScopeCount; ++i) {
      const nsTArray<nsString>& deleted = *(aDirtyData.mChanged[i].mDeletedKeys);
      size_t deletedKeyCount = deleted.Length();
      for (uint32_t j = 0; j < deletedKeyCount; ++j) {
        rv = BindScopeAndKey(deleteKeyStmt,
                             aDirtyData.mScopeNames[i],
                             deleted[j],
                             getter_AddRefs(tempArray));
        NS_ENSURE_SUCCESS(rv, rv);
        mFlushStatements.AppendElement(deleteKeyStmt);
        mFlushStatementParams.AppendElement(tempArray);
      }
    }
  }

  
  for (uint32_t i = 0; i < dirtyScopeCount; ++i) {
    const nsTArray<nsString>& dirtyKeys = aDirtyData.mChanged[i].mDirtyKeys;
    const nsTArray<nsScopeCache::KeyEntry*>& dirtyValues =
      aDirtyData.mChanged[i].mDirtyValues;
    size_t dirtyKeyCount = dirtyKeys.Length();
    for (uint32_t j = 0; j < dirtyKeyCount; ++j) {
      nsresult rv = BindInsertKey(insertKeyStmt,
                                  aDirtyData.mScopeNames[i],
                                  dirtyKeys[j],
                                  dirtyValues[j],
                                  getter_AddRefs(tempArray));
      NS_ENSURE_SUCCESS(rv, rv);
      mFlushStatements.AppendElement(insertKeyStmt);
      mFlushStatementParams.AppendElement(tempArray);
    }
  }

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::PrepareForFlush()
{
  
  FlushData dirtyData;
  mCache.GetFlushData(&dirtyData);
  if (!dirtyData.mChanged.Length() && !mWasRemoveAllCalled) {
    return NS_OK;
  }

  
  nsresult rv = PrepareFlushStatements(dirtyData);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mCache.MarkScopesPending();
  mIsRemoveAllPending = mWasRemoveAllCalled;
  mWasRemoveAllCalled = false;
  mIsFlushPending = true;

  return NS_OK;
}





nsresult
nsDOMStoragePersistentDB::FlushAndEvictFromCache(bool aIsShuttingDown)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  EvictUnusedScopes();

  
  if (mIsFlushPending && !aIsShuttingDown) {
    return NS_OK;
  }

  if (mFlushThread && aIsShuttingDown && mIsFlushPending) {
    
    
    
    mFlushThread->Shutdown();
    mFlushThread = nullptr;

    
    nsresult rv = PrepareForFlush();
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = Flush();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mFlushThread && !aIsShuttingDown) {
    
    nsresult rv = PrepareForFlush();
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!mIsFlushPending) {
      return NS_OK;
    }

    
    nsRefPtr<FlushCallbackTask> callbackTask = new FlushCallbackTask(this);
    nsRefPtr<FlushTask> flushTask = new FlushTask(this, callbackTask);

    rv = mFlushThread->Dispatch(flushTask, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      
      HandleFlushComplete(false);
    }
  } else {
    
    
    
    
    nsresult rv = PrepareForFlush();
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!mIsFlushPending) {
      return NS_OK;
    }

    rv = Flush();
    HandleFlushComplete(NS_SUCCEEDED(rv));
  }

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::GetAllKeys(DOMStorageImpl* aStorage,
                                     nsTHashtable<nsSessionStorageEntry>* aKeys)
{
  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_GETALLKEYS_MS> timer;

  nsresult rv = EnsureScopeLoaded(aStorage);
  NS_ENSURE_SUCCESS(rv, rv);

  nsScopeCache* scopeCache = mCache.GetScope(aStorage->GetScopeDBKey());
  MOZ_ASSERT(scopeCache);
  rv = scopeCache->GetAllKeys(aStorage, aKeys);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::GetKeyValue(DOMStorageImpl* aStorage,
                                      const nsAString& aKey,
                                      nsAString& aValue,
                                      bool* aSecure)
{
  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_GETVALUE_MS> timer;
  PROFILER_LABEL("nsDOMStoragePersistentDB", "GetKeyValue");

  nsresult rv = EnsureScopeLoaded(aStorage);
  NS_ENSURE_SUCCESS(rv, rv);

  nsScopeCache* scopeCache = mCache.GetScope(aStorage->GetScopeDBKey());
  MOZ_ASSERT(scopeCache);
  if (scopeCache->GetKey(aKey, aValue, aSecure)) {
    return NS_OK;
  } else {
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }
}

nsresult
nsDOMStoragePersistentDB::SetKey(DOMStorageImpl* aStorage,
                                 const nsAString& aKey,
                                 const nsAString& aValue,
                                 bool aSecure)
{
  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_SETVALUE_MS> timer;

  nsresult rv = EnsureScopeLoaded(aStorage);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t usage = 0;
  if (!aStorage->GetQuotaDBKey().IsEmpty()) {
    rv = GetUsage(aStorage, &usage);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  usage += aKey.Length() + aValue.Length();

  nsScopeCache* scopeCache = mCache.GetScope(aStorage->GetScopeDBKey());
  MOZ_ASSERT(scopeCache);

  nsAutoString previousValue;
  bool secure;
  bool keyExists = scopeCache->GetKey(aKey, previousValue, &secure);
  if (keyExists) {
    if (!aSecure && secure) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }
    usage -= aKey.Length() + previousValue.Length();
  }

  if (usage > GetQuota()) {
    return NS_ERROR_DOM_QUOTA_REACHED;
  }

  rv = scopeCache->SetKey(aKey, aValue, aSecure);
  NS_ENSURE_SUCCESS(rv, rv);

  MarkScopeDirty(aStorage);

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::SetSecure(DOMStorageImpl* aStorage,
                                    const nsAString& aKey,
                                    const bool aSecure)
{
  nsresult rv = EnsureScopeLoaded(aStorage);
  NS_ENSURE_SUCCESS(rv, rv);

  nsScopeCache* scopeCache = mCache.GetScope(aStorage->GetScopeDBKey());
  MOZ_ASSERT(scopeCache);

  scopeCache->SetSecure(aKey, aSecure);

  MarkScopeDirty(aStorage);

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::RemoveKey(DOMStorageImpl* aStorage,
                                    const nsAString& aKey)
{
  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_REMOVEKEY_MS> timer;

  nsresult rv = EnsureScopeLoaded(aStorage);
  NS_ENSURE_SUCCESS(rv, rv);

  nsScopeCache* scopeCache = mCache.GetScope(aStorage->GetScopeDBKey());
  MOZ_ASSERT(scopeCache);

  scopeCache->RemoveKey(aKey);

  MarkScopeDirty(aStorage);

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::ClearStorage(DOMStorageImpl* aStorage)
{
  nsAutoCString scopeKey(aStorage->GetScopeDBKey());
  nsAutoCString quotaKey(aStorage->GetQuotaDBKey());

  
  
  bool isScopeCached = mCache.IsScopeCached(scopeKey);

  int32_t usage;
  bool isSiteUsageCached = !quotaKey.IsEmpty() &&
                           mQuotaUseByUncached.Get(quotaKey, &usage);

  if (!isScopeCached && isSiteUsageCached) {
    mQuotaUseByUncached.Remove(quotaKey);
  }

  nsScopeCache* scopeCache = mCache.GetScope(scopeKey);
  if (!scopeCache) {
    
    scopeCache = new nsScopeCache();
    mCache.AddScope(scopeKey, scopeCache);
    DOMStorageImpl::gStorageDB->EnsureCacheFlushTimer();
  }

  
  scopeCache->DeleteScope();

  MarkScopeDirty(aStorage);

  return NS_OK;
}

static PLDHashOperator
InvalidateMatchingQuotaEnum(const nsACString& aQuotaKey,
                            int32_t&,
                            void* aPattern)
{
  const nsACString* pattern = static_cast<nsACString*>(aPattern);
  if (StringBeginsWith(*pattern, aQuotaKey)) {
    return PL_DHASH_REMOVE;
  } else {
    return PL_DHASH_NEXT;
  }
}

nsresult
nsDOMStoragePersistentDB::FetchMatchingScopeNames(const nsACString& aPattern)
{
  nsCOMPtr<mozIStorageStatement> stmt = mReadStatements.GetCachedStatement(
    "SELECT DISTINCT(scope) FROM webappsstore2"
    " WHERE scope LIKE :scope"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scope(stmt);

  nsresult rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("scope"),
                                           aPattern + NS_LITERAL_CSTRING("%"));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  while (NS_SUCCEEDED(rv = stmt->ExecuteStep(&exists)) && exists) {
    nsAutoCString scopeName;
    rv = stmt->GetUTF8String(0, scopeName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsScopeCache* scopeCache = mCache.GetScope(scopeName);
    if (!scopeCache) {
      scopeCache = new nsScopeCache();
      mCache.AddScope(scopeName, scopeCache);
    }
  }

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::RemoveOwner(const nsACString& aOwner)
{
  nsAutoCString subdomainsDBKey;
  nsDOMStorageDBWrapper::CreateReversedDomain(aOwner, subdomainsDBKey);

  
  
  nsresult rv = FetchMatchingScopeNames(subdomainsDBKey);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mCache.MarkMatchingScopesDeleted(subdomainsDBKey);

  
  mQuotaUseByUncached.Enumerate(InvalidateMatchingQuotaEnum, &subdomainsDBKey);

  MarkAllScopesDirty();

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::RemoveAllForApp(uint32_t aAppId, bool aOnlyBrowserElement)
{
  nsAutoCString appIdString;
  appIdString.AppendInt(aAppId);

  nsAutoCString scopePattern = appIdString;
  if (aOnlyBrowserElement) {
    scopePattern.AppendLiteral(":t:");
  } else {
    scopePattern.AppendLiteral(":_:");
  }

  
  
  nsresult rv = FetchMatchingScopeNames(scopePattern);
  NS_ENSURE_SUCCESS(rv, rv);

  
  scopePattern = appIdString;
  if (aOnlyBrowserElement) {
    scopePattern.AppendLiteral(":t:");
  } else {
    scopePattern.AppendLiteral(":");
  }
  mCache.MarkMatchingScopesDeleted(scopePattern);

  
  mQuotaUseByUncached.Enumerate(InvalidateMatchingQuotaEnum, &scopePattern);

  MarkAllScopesDirty();

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::RemoveAll()
{
  Telemetry::AutoTimer<Telemetry::LOCALDOMSTORAGE_REMOVEALL_MS> timer;

  mCache.ForgetAllScopes();
  mQuotaUseByUncached.Clear();
  mWasRemoveAllCalled = true;

  MarkAllScopesDirty();

  DOMStorageImpl::gStorageDB->EnsureCacheFlushTimer();

  return NS_OK;
}

nsresult
nsDOMStoragePersistentDB::GetUsage(DOMStorageImpl* aStorage,
                                   int32_t *aUsage)
{
  return GetUsageInternal(aStorage->GetQuotaDBKey(), aUsage);
}

nsresult
nsDOMStoragePersistentDB::GetUsage(const nsACString& aDomain,
                                   int32_t *aUsage)
{
  nsAutoCString quotaKey;
  nsresult rv = nsDOMStorageDBWrapper::CreateQuotaDBKey(aDomain, quotaKey);
  NS_ENSURE_SUCCESS(rv, rv);

  return GetUsageInternal(quotaKey, aUsage);
}

nsresult
nsDOMStoragePersistentDB::GetUsageInternal(const nsACString& aQuotaKey,
                                           int32_t *aUsage)
{
  nsresult rv = EnsureQuotaUsageLoaded(aQuotaKey);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t uncachedSize = 0;
  DebugOnly<bool> found = mQuotaUseByUncached.Get(aQuotaKey, &uncachedSize);
  MOZ_ASSERT(found);

  int32_t cachedSize = mCache.GetQuotaUsage(aQuotaKey);

  *aUsage = uncachedSize + cachedSize;

  return NS_OK;
}

