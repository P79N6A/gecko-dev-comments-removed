




#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsCRT.h"
#include "nsICryptoHash.h"
#include "nsICryptoHMAC.h"
#include "nsIDirectoryService.h"
#include "nsIKeyModule.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIProperties.h"
#include "nsToolkitCompsCID.h"
#include "nsIUrlClassifierUtils.h"
#include "nsIXULRuntime.h"
#include "nsUrlClassifierDBService.h"
#include "nsUrlClassifierUtils.h"
#include "nsUrlClassifierProxies.h"
#include "nsURILoader.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsTArray.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsThreadUtils.h"
#include "nsXPCOMStrings.h"
#include "nsProxyRelease.h"
#include "nsString.h"
#include "mozilla/Atomics.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Mutex.h"
#include "mozilla/Preferences.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Telemetry.h"
#include "prlog.h"
#include "prprf.h"
#include "prnetdb.h"
#include "Entries.h"
#include "HashStore.h"
#include "Classifier.h"
#include "ProtocolParser.h"
#include "mozilla/Attributes.h"
#include "nsIPrincipal.h"
#include "Classifier.h"
#include "ProtocolParser.h"
#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::safebrowsing;


#if defined(PR_LOGGING)
PRLogModuleInfo *gUrlClassifierDbServiceLog = nullptr;
#define LOG(args) PR_LOG(gUrlClassifierDbServiceLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gUrlClassifierDbServiceLog, 4)
#else
#define LOG(args)
#define LOG_ENABLED() (false)
#endif


#define CHECK_MALWARE_PREF      "browser.safebrowsing.malware.enabled"
#define CHECK_MALWARE_DEFAULT   false

#define CHECK_PHISHING_PREF     "browser.safebrowsing.enabled"
#define CHECK_PHISHING_DEFAULT  false

#define CHECK_TRACKING_PREF     "privacy.trackingprotection.enabled"
#define CHECK_TRACKING_DEFAULT  false

#define CHECK_TRACKING_PB_PREF    "privacy.trackingprotection.pbmode.enabled"
#define CHECK_TRACKING_PB_DEFAULT false

#define GETHASH_NOISE_PREF      "urlclassifier.gethashnoise"
#define GETHASH_NOISE_DEFAULT   4


#define MALWARE_TABLE_PREF      "urlclassifier.malwareTable"
#define PHISH_TABLE_PREF        "urlclassifier.phishTable"
#define TRACKING_TABLE_PREF     "urlclassifier.trackingTable"
#define DOWNLOAD_BLOCK_TABLE_PREF "urlclassifier.downloadBlockTable"
#define DOWNLOAD_ALLOW_TABLE_PREF "urlclassifier.downloadAllowTable"
#define DISALLOW_COMPLETION_TABLE_PREF "urlclassifier.disallow_completions"

#define CONFIRM_AGE_PREF        "urlclassifier.max-complete-age"
#define CONFIRM_AGE_DEFAULT_SEC (45 * 60)

class nsUrlClassifierDBServiceWorker;


static nsUrlClassifierDBService* sUrlClassifierDBService;

nsIThread* nsUrlClassifierDBService::gDbBackgroundThread = nullptr;



static bool gShuttingDownThread = false;

static mozilla::Atomic<int32_t> gFreshnessGuarantee(CONFIRM_AGE_DEFAULT_SEC);

NS_IMPL_ISUPPORTS(nsUrlClassifierDBServiceWorker,
                  nsIUrlClassifierDBServiceWorker,
                  nsIUrlClassifierDBService)

nsUrlClassifierDBServiceWorker::nsUrlClassifierDBServiceWorker()
  : mInStream(false)
  , mGethashNoise(0)
  , mPendingLookupLock("nsUrlClassifierDBServerWorker.mPendingLookupLock")
{
}

nsUrlClassifierDBServiceWorker::~nsUrlClassifierDBServiceWorker()
{
  NS_ASSERTION(!mClassifier,
               "Db connection not closed, leaking memory!  Call CloseDb "
               "to close the connection.");
}

nsresult
nsUrlClassifierDBServiceWorker::Init(uint32_t aGethashNoise,
                                     nsCOMPtr<nsIFile> aCacheDir)
{
  mGethashNoise = aGethashNoise;
  mCacheDir = aCacheDir;

  ResetUpdate();

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::QueueLookup(const nsACString& spec,
                                            const nsACString& tables,
                                            nsIUrlClassifierLookupCallback* callback)
{
  MutexAutoLock lock(mPendingLookupLock);

  PendingLookup* lookup = mPendingLookups.AppendElement();
  if (!lookup) return NS_ERROR_OUT_OF_MEMORY;

  lookup->mStartTime = TimeStamp::Now();
  lookup->mKey = spec;
  lookup->mCallback = callback;
  lookup->mTables = tables;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::DoLocalLookup(const nsACString& spec,
                                              const nsACString& tables,
                                              LookupResultArray* results)
{
  MOZ_ASSERT(!NS_IsMainThread(), "DoLocalLookup must be on background thread");
  if (!results) {
    return NS_ERROR_FAILURE;
  }
  
  if (!mClassifier) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  mClassifier->Check(spec, tables, gFreshnessGuarantee, *results);

  LOG(("Found %d results.", results->Length()));
  return NS_OK;
}

static nsresult
TablesToResponse(const nsACString& tables)
{
  
  
  if (FindInReadable(NS_LITERAL_CSTRING("-malware-"), tables)) {
    return NS_ERROR_MALWARE_URI;
  }
  if (FindInReadable(NS_LITERAL_CSTRING("-phish-"), tables)) {
    return NS_ERROR_PHISHING_URI;
  }
  if (FindInReadable(NS_LITERAL_CSTRING("-track-"), tables)) {
    return NS_ERROR_TRACKING_URI;
  }
  if (FindInReadable(NS_LITERAL_CSTRING("-unwanted-"), tables)) {
    return NS_ERROR_UNWANTED_URI;
  }
  return NS_OK;
}

static nsCString
ProcessLookupResults(LookupResultArray* results)
{
  
  nsTArray<nsCString> tables;
  for (uint32_t i = 0; i < results->Length(); i++) {
    LookupResult& result = results->ElementAt(i);
    MOZ_ASSERT(!result.mNoise, "Lookup results should not have noise added");
    LOG(("Found result from table %s", result.mTableName.get()));
    if (tables.IndexOf(result.mTableName) == nsTArray<nsCString>::NoIndex) {
      tables.AppendElement(result.mTableName);
    }
  }
  nsAutoCString tableStr;
  for (uint32_t i = 0; i < tables.Length(); i++) {
    if (i != 0)
      tableStr.Append(',');
    tableStr.Append(tables[i]);
  }
  return tableStr;
}












nsresult
nsUrlClassifierDBServiceWorker::DoLookup(const nsACString& spec,
                                         const nsACString& tables,
                                         nsIUrlClassifierLookupCallback* c)
{
  if (gShuttingDownThread) {
    c->LookupComplete(nullptr);
    return NS_ERROR_NOT_INITIALIZED;
  }

#if defined(PR_LOGGING)
  PRIntervalTime clockStart = 0;
  if (LOG_ENABLED()) {
    clockStart = PR_IntervalNow();
  }
#endif

  nsAutoPtr<LookupResultArray> results(new LookupResultArray());
  if (!results) {
    c->LookupComplete(nullptr);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = DoLocalLookup(spec, tables, results);
  if (NS_FAILED(rv)) {
    c->LookupComplete(nullptr);
    return rv;
  }

  LOG(("Found %d results.", results->Length()));


#if defined(PR_LOGGING)
  if (LOG_ENABLED()) {
    PRIntervalTime clockEnd = PR_IntervalNow();
    LOG(("query took %dms\n",
         PR_IntervalToMilliseconds(clockEnd - clockStart)));
  }
#endif

  nsAutoPtr<LookupResultArray> completes(new LookupResultArray());

  for (uint32_t i = 0; i < results->Length(); i++) {
    if (!mMissCache.Contains(results->ElementAt(i).hash.prefix)) {
      completes->AppendElement(results->ElementAt(i));
    }
  }

  for (uint32_t i = 0; i < completes->Length(); i++) {
    if (!completes->ElementAt(i).Confirmed()) {
      
      
      
      AddNoise(completes->ElementAt(i).hash.prefix,
               completes->ElementAt(i).mTableName,
               mGethashNoise, *completes);
      break;
    }
  }

  
  c->LookupComplete(completes.forget());

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::HandlePendingLookups()
{
  MutexAutoLock lock(mPendingLookupLock);
  while (mPendingLookups.Length() > 0) {
    PendingLookup lookup = mPendingLookups[0];
    mPendingLookups.RemoveElementAt(0);
    {
      MutexAutoUnlock unlock(mPendingLookupLock);
      DoLookup(lookup.mKey, lookup.mTables, lookup.mCallback);
    }
    double lookupTime = (TimeStamp::Now() - lookup.mStartTime).ToMilliseconds();
    Telemetry::Accumulate(Telemetry::URLCLASSIFIER_LOOKUP_TIME,
                          static_cast<uint32_t>(lookupTime));
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::AddNoise(const Prefix aPrefix,
                                         const nsCString tableName,
                                         uint32_t aCount,
                                         LookupResultArray& results)
{
  if (aCount < 1) {
    return NS_OK;
  }

  PrefixArray noiseEntries;
  nsresult rv = mClassifier->ReadNoiseEntries(aPrefix, tableName,
                                              aCount, &noiseEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  for (uint32_t i = 0; i < noiseEntries.Length(); i++) {
    LookupResult *result = results.AppendElement();
    if (!result)
      return NS_ERROR_OUT_OF_MEMORY;

    result->hash.prefix = noiseEntries[i];
    result->mNoise = true;

    result->mTableName.Assign(tableName);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::Lookup(nsIPrincipal* aPrincipal,
                                       const nsACString& aTables,
                                       nsIUrlClassifierCallback* c)
{
  return HandlePendingLookups();
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::GetTables(nsIUrlClassifierCallback* c)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open SafeBrowsing database");
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString response;
  mClassifier->TableRequest(response);
  LOG(("GetTables: %s", response.get()));
  c->HandleEvent(response);

  return rv;
}

void
nsUrlClassifierDBServiceWorker::ResetStream()
{
  LOG(("ResetStream"));
  mInStream = false;
  mProtocolParser = nullptr;
}

void
nsUrlClassifierDBServiceWorker::ResetUpdate()
{
  LOG(("ResetUpdate"));
  mUpdateWait = 0;
  mUpdateStatus = NS_OK;
  mUpdateObserver = nullptr;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::SetHashCompleter(const nsACString &tableName,
                                                 nsIUrlClassifierHashCompleter *completer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::BeginUpdate(nsIUrlClassifierUpdateObserver *observer,
                                            const nsACString &tables)
{
  LOG(("nsUrlClassifierDBServiceWorker::BeginUpdate [%s]", PromiseFlatCString(tables).get()));

  if (gShuttingDownThread) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  NS_ENSURE_STATE(!mUpdateObserver);

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open SafeBrowsing database");
    return NS_ERROR_FAILURE;
  }

  mUpdateStatus = NS_OK;
  mUpdateObserver = observer;
  Classifier::SplitTables(tables, mUpdateTables);

  return NS_OK;
}


NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::BeginStream(const nsACString &table)
{
  LOG(("nsUrlClassifierDBServiceWorker::BeginStream"));
  MOZ_ASSERT(!NS_IsMainThread(), "Streaming must be on the background thread");

  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(mUpdateObserver);
  NS_ENSURE_STATE(!mInStream);

  mInStream = true;

  NS_ASSERTION(!mProtocolParser, "Should not have a protocol parser.");

  mProtocolParser = new ProtocolParser();
  if (!mProtocolParser)
    return NS_ERROR_OUT_OF_MEMORY;

  mProtocolParser->Init(mCryptoHash);

  if (!table.IsEmpty()) {
    mProtocolParser->SetCurrentTable(table);
  }

  return NS_OK;
}
































NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::UpdateStream(const nsACString& chunk)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(mInStream);

  HandlePendingLookups();

  
  return mProtocolParser->AppendStream(chunk);
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::FinishStream()
{
  if (gShuttingDownThread) {
    LOG(("shutting down"));
    return NS_ERROR_NOT_INITIALIZED;
  }

  NS_ENSURE_STATE(mInStream);
  NS_ENSURE_STATE(mUpdateObserver);

  mInStream = false;

  if (NS_SUCCEEDED(mProtocolParser->Status())) {
    if (mProtocolParser->UpdateWait()) {
      mUpdateWait = mProtocolParser->UpdateWait();
    }
    
    const nsTArray<ProtocolParser::ForwardedUpdate> &forwards =
      mProtocolParser->Forwards();
    for (uint32_t i = 0; i < forwards.Length(); i++) {
      const ProtocolParser::ForwardedUpdate &forward = forwards[i];
      mUpdateObserver->UpdateUrlRequested(forward.url, forward.table);
    }
    
    
    mTableUpdates.AppendElements(mProtocolParser->GetTableUpdates());
    mProtocolParser->ForgetTableUpdates();
  } else {
    mUpdateStatus = mProtocolParser->Status();
  }
  mUpdateObserver->StreamFinished(mProtocolParser->Status(), 0);

  if (NS_SUCCEEDED(mUpdateStatus)) {
    if (mProtocolParser->ResetRequested()) {
      mClassifier->Reset();
   }
  }

  mProtocolParser = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::FinishUpdate()
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;
  NS_ENSURE_STATE(mUpdateObserver);

  if (NS_SUCCEEDED(mUpdateStatus)) {
    mUpdateStatus = ApplyUpdate();
  }

  mMissCache.Clear();

  if (NS_SUCCEEDED(mUpdateStatus)) {
    LOG(("Notifying success: %d", mUpdateWait));
    mUpdateObserver->UpdateSuccess(mUpdateWait);
  } else {
    LOG(("Notifying error: %d", mUpdateStatus));
    mUpdateObserver->UpdateError(mUpdateStatus);
    



    mClassifier->MarkSpoiled(mUpdateTables);
  }
  mUpdateObserver = nullptr;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ApplyUpdate()
{
  LOG(("nsUrlClassifierDBServiceWorker::ApplyUpdate()"));
  return mClassifier->ApplyUpdates(&mTableUpdates);
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::ResetDatabase()
{
  nsresult rv = OpenDb();

  if (NS_SUCCEEDED(rv)) {
    mClassifier->Reset();
  }

  rv = CloseDb();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CancelUpdate()
{
  LOG(("nsUrlClassifierDBServiceWorker::CancelUpdate"));

  if (mUpdateObserver) {
    LOG(("UpdateObserver exists, cancelling"));

    mUpdateStatus = NS_BINDING_ABORTED;

    mUpdateObserver->UpdateError(mUpdateStatus);

    



    mClassifier->MarkSpoiled(mUpdateTables);

    ResetStream();
    ResetUpdate();
  } else {
    LOG(("No UpdateObserver, nothing to cancel"));
  }

  return NS_OK;
}





NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CloseDb()
{
  if (mClassifier) {
    mClassifier->Close();
    mClassifier = nullptr;
  }

  mCryptoHash = nullptr;
  LOG(("urlclassifier db closed\n"));

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CacheCompletions(CacheResultArray *results)
{
  LOG(("nsUrlClassifierDBServiceWorker::CacheCompletions [%p]", this));
  if (!mClassifier)
    return NS_OK;

  
  nsAutoPtr<CacheResultArray> resultsPtr(results);

  nsAutoPtr<ProtocolParser> pParse(new ProtocolParser());
  nsTArray<TableUpdate*> updates;

  
  
  
  nsTArray<nsCString> tables;
  nsresult rv = mClassifier->ActiveTables(tables);
  NS_ENSURE_SUCCESS(rv, rv);

  for (uint32_t i = 0; i < resultsPtr->Length(); i++) {
    bool activeTable = false;
    for (uint32_t table = 0; table < tables.Length(); table++) {
      if (tables[table].Equals(resultsPtr->ElementAt(i).table)) {
        activeTable = true;
        break;
      }
    }
    if (activeTable) {
      TableUpdate * tu = pParse->GetTableUpdate(resultsPtr->ElementAt(i).table);
      LOG(("CacheCompletion Addchunk %d hash %X", resultsPtr->ElementAt(i).entry.addChunk,
           resultsPtr->ElementAt(i).entry.ToUint32()));
      rv = tu->NewAddComplete(resultsPtr->ElementAt(i).entry.addChunk,
                              resultsPtr->ElementAt(i).entry.complete);
      if (NS_FAILED(rv)) {
        
        
        return rv;
      }
      rv = tu->NewAddChunk(resultsPtr->ElementAt(i).entry.addChunk);
      if (NS_FAILED(rv)) {
        return rv;
      }
      tu->SetLocalUpdate();
      updates.AppendElement(tu);
      pParse->ForgetTableUpdates();
    } else {
      LOG(("Completion received, but table is not active, so not caching."));
    }
   }

  mClassifier->ApplyUpdates(&updates);
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CacheMisses(PrefixArray *results)
{
  LOG(("nsUrlClassifierDBServiceWorker::CacheMisses [%p] %d",
       this, results->Length()));

  
  nsAutoPtr<PrefixArray> resultsPtr(results);

  for (uint32_t i = 0; i < resultsPtr->Length(); i++) {
    mMissCache.AppendElement(resultsPtr->ElementAt(i));
   }
  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::OpenDb()
{
  MOZ_ASSERT(!NS_IsMainThread(), "Must initialize DB on background thread");
  
  if (mClassifier) {
    return NS_OK;
  }

  nsresult rv;
  mCryptoHash = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<Classifier> classifier(new Classifier());
  if (!classifier) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = classifier->Open(*mCacheDir);
  NS_ENSURE_SUCCESS(rv, rv);

  mClassifier = classifier;

  return NS_OK;
}








class nsUrlClassifierLookupCallback final : public nsIUrlClassifierLookupCallback
                                          , public nsIUrlClassifierHashCompleterCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERLOOKUPCALLBACK
  NS_DECL_NSIURLCLASSIFIERHASHCOMPLETERCALLBACK

  nsUrlClassifierLookupCallback(nsUrlClassifierDBService *dbservice,
                                nsIUrlClassifierCallback *c)
    : mDBService(dbservice)
    , mResults(nullptr)
    , mPendingCompletions(0)
    , mCallback(c)
    {}

private:
  ~nsUrlClassifierLookupCallback();

  nsresult HandleResults();

  nsRefPtr<nsUrlClassifierDBService> mDBService;
  nsAutoPtr<LookupResultArray> mResults;

  
  nsAutoPtr<CacheResultArray> mCacheResults;

  uint32_t mPendingCompletions;
  nsCOMPtr<nsIUrlClassifierCallback> mCallback;
};

NS_IMPL_ISUPPORTS(nsUrlClassifierLookupCallback,
                  nsIUrlClassifierLookupCallback,
                  nsIUrlClassifierHashCompleterCallback)

nsUrlClassifierLookupCallback::~nsUrlClassifierLookupCallback()
{
  nsCOMPtr<nsIThread> thread;
  (void)NS_GetMainThread(getter_AddRefs(thread));

  if (mCallback) {
    (void)NS_ProxyRelease(thread, mCallback, false);
  }
}

NS_IMETHODIMP
nsUrlClassifierLookupCallback::LookupComplete(nsTArray<LookupResult>* results)
{
  NS_ASSERTION(mResults == nullptr,
               "Should only get one set of results per nsUrlClassifierLookupCallback!");

  if (!results) {
    HandleResults();
    return NS_OK;
  }

  mResults = results;

  
  for (uint32_t i = 0; i < results->Length(); i++) {
    LookupResult& result = results->ElementAt(i);

    
    if (!result.Confirmed()) {
      nsCOMPtr<nsIUrlClassifierHashCompleter> completer;
      nsCString gethashUrl;
      nsresult rv;
      nsCOMPtr<nsIUrlListManager> listManager = do_GetService(
        "@mozilla.org/url-classifier/listmanager;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = listManager->GetGethashUrl(result.mTableName, gethashUrl);
      NS_ENSURE_SUCCESS(rv, rv);
      
      
      
      
      if ((!gethashUrl.IsEmpty() ||
           StringBeginsWith(result.mTableName, NS_LITERAL_CSTRING("test-"))) &&
          mDBService->GetCompleter(result.mTableName,
                                   getter_AddRefs(completer))) {
        nsAutoCString partialHash;
        partialHash.Assign(reinterpret_cast<char*>(&result.hash.prefix),
                           PREFIX_SIZE);

        nsresult rv = completer->Complete(partialHash, gethashUrl, this);
        if (NS_SUCCEEDED(rv)) {
          mPendingCompletions++;
        }
      } else {
        
        
        
        if (result.Complete()) {
          result.mFresh = true;
        } else {
          NS_WARNING("Partial match in a table without a valid completer, ignoring partial match.");
        }
      }
    }
  }

  if (mPendingCompletions == 0) {
    
    HandleResults();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierLookupCallback::CompletionFinished(nsresult status)
{
  LOG(("nsUrlClassifierLookupCallback::CompletionFinished [%p, %08x]",
       this, status));
  if (NS_FAILED(status)) {
    NS_WARNING("gethash response failed.");
  }

  mPendingCompletions--;
  if (mPendingCompletions == 0) {
    HandleResults();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierLookupCallback::Completion(const nsACString& completeHash,
                                          const nsACString& tableName,
                                          uint32_t chunkId)
{
  LOG(("nsUrlClassifierLookupCallback::Completion [%p, %s, %d]",
       this, PromiseFlatCString(tableName).get(), chunkId));
  mozilla::safebrowsing::Completion hash;
  hash.Assign(completeHash);

  
  if (!mCacheResults) {
    mCacheResults = new CacheResultArray();
    if (!mCacheResults)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  CacheResult result;
  result.entry.addChunk = chunkId;
  result.entry.complete = hash;
  result.table = tableName;

  
  mCacheResults->AppendElement(result);

  
  for (uint32_t i = 0; i < mResults->Length(); i++) {
    LookupResult& result = mResults->ElementAt(i);

    
    if (result.CompleteHash() == hash && result.mTableName.Equals(tableName)) {
      result.mProtocolConfirmed = true;
    }
  }

  return NS_OK;
}

nsresult
nsUrlClassifierLookupCallback::HandleResults()
{
  if (!mResults) {
    
    return mCallback->HandleEvent(NS_LITERAL_CSTRING(""));
  }

  nsTArray<nsCString> tables;
  
  for (uint32_t i = 0; i < mResults->Length(); i++) {
    LookupResult& result = mResults->ElementAt(i);

    
    
    
    if (!result.Confirmed() || result.mNoise) {
      LOG(("Skipping result from table %s", result.mTableName.get()));
      continue;
    }

    LOG(("Confirmed result from table %s", result.mTableName.get()));

    if (tables.IndexOf(result.mTableName) == nsTArray<nsCString>::NoIndex) {
      tables.AppendElement(result.mTableName);
    }
  }

  
  
  
  
  nsAutoPtr<PrefixArray> cacheMisses(new PrefixArray());
  if (cacheMisses) {
    for (uint32_t i = 0; i < mResults->Length(); i++) {
      LookupResult &result = mResults->ElementAt(i);
      if (!result.Confirmed() && !result.mNoise) {
        cacheMisses->AppendElement(result.PrefixHash());
      }
    }
    
    mDBService->CacheMisses(cacheMisses.forget());
  }

  if (mCacheResults) {
    
    
    mDBService->CacheCompletions(mCacheResults.forget());
  }

  nsAutoCString tableStr;
  for (uint32_t i = 0; i < tables.Length(); i++) {
    if (i != 0)
      tableStr.Append(',');
    tableStr.Append(tables[i]);
  }

  return mCallback->HandleEvent(tableStr);
}






class nsUrlClassifierClassifyCallback final : public nsIUrlClassifierCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERCALLBACK

  nsUrlClassifierClassifyCallback(nsIURIClassifierCallback *c,
                                  bool checkMalware,
                                  bool checkPhishing,
                                  bool checkTracking)
    : mCallback(c)
    {}

private:
  ~nsUrlClassifierClassifyCallback() {}

  nsCOMPtr<nsIURIClassifierCallback> mCallback;
};

NS_IMPL_ISUPPORTS(nsUrlClassifierClassifyCallback,
                  nsIUrlClassifierCallback)

NS_IMETHODIMP
nsUrlClassifierClassifyCallback::HandleEvent(const nsACString& tables)
{
  nsresult response = TablesToResponse(tables);
  mCallback->OnClassifyComplete(response);
  return NS_OK;
}





NS_IMPL_ISUPPORTS(nsUrlClassifierDBService,
                  nsIUrlClassifierDBService,
                  nsIURIClassifier,
                  nsIObserver)

 nsUrlClassifierDBService*
nsUrlClassifierDBService::GetInstance(nsresult *result)
{
  *result = NS_OK;
  if (!sUrlClassifierDBService) {
    sUrlClassifierDBService = new nsUrlClassifierDBService();
    if (!sUrlClassifierDBService) {
      *result = NS_ERROR_OUT_OF_MEMORY;
      return nullptr;
    }

    NS_ADDREF(sUrlClassifierDBService);   

    *result = sUrlClassifierDBService->Init();
    if (NS_FAILED(*result)) {
      NS_RELEASE(sUrlClassifierDBService);
      return nullptr;
    }
  } else {
    
    NS_ADDREF(sUrlClassifierDBService);   
  }
  return sUrlClassifierDBService;
}


nsUrlClassifierDBService::nsUrlClassifierDBService()
 : mCheckMalware(CHECK_MALWARE_DEFAULT)
 , mCheckPhishing(CHECK_PHISHING_DEFAULT)
 , mCheckTracking(CHECK_TRACKING_DEFAULT)
 , mInUpdate(false)
{
}

nsUrlClassifierDBService::~nsUrlClassifierDBService()
{
  sUrlClassifierDBService = nullptr;
}

nsresult
nsUrlClassifierDBService::ReadTablesFromPrefs()
{
  nsCString allTables;
  nsCString tables;
  Preferences::GetCString(PHISH_TABLE_PREF, &allTables);

  Preferences::GetCString(MALWARE_TABLE_PREF, &tables);
  if (!tables.IsEmpty()) {
    allTables.Append(',');
    allTables.Append(tables);
  }

  Preferences::GetCString(DOWNLOAD_BLOCK_TABLE_PREF, &tables);
  if (!tables.IsEmpty()) {
    allTables.Append(',');
    allTables.Append(tables);
  }

  Preferences::GetCString(DOWNLOAD_ALLOW_TABLE_PREF, &tables);
  if (!tables.IsEmpty()) {
    allTables.Append(',');
    allTables.Append(tables);
  }

  Preferences::GetCString(TRACKING_TABLE_PREF, &tables);
  if (!tables.IsEmpty()) {
    allTables.Append(',');
    allTables.Append(tables);
  }

  Classifier::SplitTables(allTables, mGethashTables);

  Preferences::GetCString(DISALLOW_COMPLETION_TABLE_PREF, &tables);
  Classifier::SplitTables(tables, mDisallowCompletionsTables);

  return NS_OK;
}

nsresult
nsUrlClassifierDBService::Init()
{
#if defined(PR_LOGGING)
  if (!gUrlClassifierDbServiceLog)
    gUrlClassifierDbServiceLog = PR_NewLogModule("UrlClassifierDbService");
#endif
  MOZ_ASSERT(NS_IsMainThread(), "Must initialize DB service on main thread");
  nsCOMPtr<nsIXULRuntime> appInfo = do_GetService("@mozilla.org/xre/app-info;1");
  if (appInfo) {
    bool inSafeMode = false;
    appInfo->GetInSafeMode(&inSafeMode);
    if (inSafeMode) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  
  mCheckMalware = Preferences::GetBool(CHECK_MALWARE_PREF,
    CHECK_MALWARE_DEFAULT);
  mCheckPhishing = Preferences::GetBool(CHECK_PHISHING_PREF,
    CHECK_PHISHING_DEFAULT);
  mCheckTracking =
    Preferences::GetBool(CHECK_TRACKING_PREF, CHECK_TRACKING_DEFAULT) ||
    Preferences::GetBool(CHECK_TRACKING_PB_PREF, CHECK_TRACKING_PB_DEFAULT);
  uint32_t gethashNoise = Preferences::GetUint(GETHASH_NOISE_PREF,
    GETHASH_NOISE_DEFAULT);
  gFreshnessGuarantee = Preferences::GetInt(CONFIRM_AGE_PREF,
    CONFIRM_AGE_DEFAULT_SEC);
  ReadTablesFromPrefs();

  
  Preferences::AddStrongObserver(this, CHECK_MALWARE_PREF);
  Preferences::AddStrongObserver(this, CHECK_PHISHING_PREF);
  Preferences::AddStrongObserver(this, CHECK_TRACKING_PREF);
  Preferences::AddStrongObserver(this, CHECK_TRACKING_PB_PREF);
  Preferences::AddStrongObserver(this, GETHASH_NOISE_PREF);
  Preferences::AddStrongObserver(this, CONFIRM_AGE_PREF);
  Preferences::AddStrongObserver(this, PHISH_TABLE_PREF);
  Preferences::AddStrongObserver(this, MALWARE_TABLE_PREF);
  Preferences::AddStrongObserver(this, TRACKING_TABLE_PREF);
  Preferences::AddStrongObserver(this, DOWNLOAD_BLOCK_TABLE_PREF);
  Preferences::AddStrongObserver(this, DOWNLOAD_ALLOW_TABLE_PREF);
  Preferences::AddStrongObserver(this, DISALLOW_COMPLETION_TABLE_PREF);

  
  nsresult rv;
  nsCOMPtr<nsICryptoHash> dummy = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> cacheDir;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                              getter_AddRefs(cacheDir));
  if (NS_FAILED(rv)) {
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(cacheDir));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  
  rv = NS_NewNamedThread("URL Classifier", &gDbBackgroundThread);
  if (NS_FAILED(rv))
    return rv;

  mWorker = new nsUrlClassifierDBServiceWorker();
  if (!mWorker)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = mWorker->Init(gethashNoise, cacheDir);
  if (NS_FAILED(rv)) {
    mWorker = nullptr;
    return rv;
  }

  
  mWorkerProxy = new UrlClassifierDBServiceWorkerProxy(mWorker);
  rv = mWorkerProxy->OpenDb();
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->AddObserver(this, "profile-before-change", false);
  observerService->AddObserver(this, "xpcom-shutdown-threads", false);

  return NS_OK;
}

void
nsUrlClassifierDBService::BuildTables(bool aTrackingProtectionEnabled,
                                      nsCString &tables)
{
  nsAutoCString malware;
  
  Preferences::GetCString(MALWARE_TABLE_PREF, &malware);
  if (mCheckMalware && !malware.IsEmpty()) {
    tables.Append(malware);
  }
  nsAutoCString phishing;
  Preferences::GetCString(PHISH_TABLE_PREF, &phishing);
  if (mCheckPhishing && !phishing.IsEmpty()) {
    tables.Append(',');
    tables.Append(phishing);
  }
  nsAutoCString tracking;
  Preferences::GetCString(TRACKING_TABLE_PREF, &tracking);
  if (aTrackingProtectionEnabled && !tracking.IsEmpty()) {
    tables.Append(',');
    tables.Append(tracking);
  }
}


NS_IMETHODIMP
nsUrlClassifierDBService::Classify(nsIPrincipal* aPrincipal,
                                   bool aTrackingProtectionEnabled,
                                   nsIURIClassifierCallback* c,
                                   bool* result)
{
  NS_ENSURE_ARG(aPrincipal);
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  if (!(mCheckMalware || mCheckPhishing)) {
    *result = false;
    return NS_OK;
  }

  nsRefPtr<nsUrlClassifierClassifyCallback> callback =
    new nsUrlClassifierClassifyCallback(c, mCheckMalware, mCheckPhishing,
                                        mCheckTracking);
  if (!callback) return NS_ERROR_OUT_OF_MEMORY;

  nsAutoCString tables;
  BuildTables(aTrackingProtectionEnabled, tables);

  nsresult rv = LookupURI(aPrincipal, tables, callback, false, result);
  if (rv == NS_ERROR_MALFORMED_URI) {
    *result = false;
    
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::ClassifyLocal(nsIPrincipal* aPrincipal,
                                        bool aTrackingProtectionEnabled,
                                        nsresult* aResponse)
{
  MOZ_ASSERT(NS_IsMainThread(), "ClassifyLocal must be on main thread");
  *aResponse = NS_OK;
  nsAutoCString tables;
  BuildTables(aTrackingProtectionEnabled, tables);
  nsAutoCString results;
  nsresult rv = ClassifyLocalWithTables(aPrincipal, tables, results);
  NS_ENSURE_SUCCESS(rv, rv);
  *aResponse = TablesToResponse(results);
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::ClassifyLocalWithTables(nsIPrincipal *aPrincipal,
                                                  const nsACString & aTables,
                                                  nsACString & aTableResults)
{
  MOZ_ASSERT(NS_IsMainThread(), "ClassifyLocalWithTables must be on main thread");

  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

  uri = NS_GetInnermostURI(uri);
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

  nsAutoCString key;
  
  nsCOMPtr<nsIUrlClassifierUtils> utilsService =
    do_GetService(NS_URLCLASSIFIERUTILS_CONTRACTID);
  rv = utilsService->GetKeyForURI(uri, key);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<LookupResultArray> results(new LookupResultArray());
  if (!results) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  rv = mWorkerProxy->DoLocalLookup(key, aTables, results);
  if (NS_SUCCEEDED(rv)) {
    aTableResults = ProcessLookupResults(results);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::Lookup(nsIPrincipal* aPrincipal,
                                 const nsACString& tables,
                                 nsIUrlClassifierCallback* c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  bool dummy;
  return LookupURI(aPrincipal, tables, c, true, &dummy);
}

nsresult
nsUrlClassifierDBService::LookupURI(nsIPrincipal* aPrincipal,
                                    const nsACString& tables,
                                    nsIUrlClassifierCallback* c,
                                    bool forceLookup,
                                    bool *didLookup)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG(aPrincipal);

  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    *didLookup = false;
    return NS_OK;
  }

  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

  uri = NS_GetInnermostURI(uri);
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

  nsAutoCString key;
  
  nsCOMPtr<nsIUrlClassifierUtils> utilsService =
    do_GetService(NS_URLCLASSIFIERUTILS_CONTRACTID);
  rv = utilsService->GetKeyForURI(uri, key);
  if (NS_FAILED(rv))
    return rv;

  if (forceLookup) {
    *didLookup = true;
  } else {
    bool clean = false;

    if (!clean) {
      nsCOMPtr<nsIPermissionManager> permissionManager =
        services::GetPermissionManager();

      if (permissionManager) {
        uint32_t perm;
        rv = permissionManager->TestPermissionFromPrincipal(aPrincipal,
                                                           "safe-browsing", &perm);
        NS_ENSURE_SUCCESS(rv, rv);

        clean |= (perm == nsIPermissionManager::ALLOW_ACTION);
      }
    }

    *didLookup = !clean;
    if (clean) {
      return NS_OK;
    }
  }

  
  
  
  nsCOMPtr<nsIUrlClassifierLookupCallback> callback =
    new nsUrlClassifierLookupCallback(this, c);
  if (!callback)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIUrlClassifierLookupCallback> proxyCallback =
    new UrlClassifierLookupCallbackProxy(callback);

  
  
  rv = mWorker->QueueLookup(key, tables, proxyCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString dummy;
  return mWorkerProxy->Lookup(nullptr, dummy, nullptr);
}

NS_IMETHODIMP
nsUrlClassifierDBService::GetTables(nsIUrlClassifierCallback* c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIUrlClassifierCallback> proxyCallback =
    new UrlClassifierCallbackProxy(c);

  return mWorkerProxy->GetTables(proxyCallback);
}

NS_IMETHODIMP
nsUrlClassifierDBService::SetHashCompleter(const nsACString &tableName,
                                           nsIUrlClassifierHashCompleter *completer)
{
  if (completer) {
    mCompleters.Put(tableName, completer);
  } else {
    mCompleters.Remove(tableName);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::BeginUpdate(nsIUrlClassifierUpdateObserver *observer,
                                      const nsACString &updateTables)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  if (mInUpdate) {
    LOG(("Already updating, not available"));
    return NS_ERROR_NOT_AVAILABLE;
  }

  mInUpdate = true;

  
  nsCOMPtr<nsIUrlClassifierUpdateObserver> proxyObserver =
    new UrlClassifierUpdateObserverProxy(observer);

  return mWorkerProxy->BeginUpdate(proxyObserver, updateTables);
}

NS_IMETHODIMP
nsUrlClassifierDBService::BeginStream(const nsACString &table)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->BeginStream(table);
}

NS_IMETHODIMP
nsUrlClassifierDBService::UpdateStream(const nsACString& aUpdateChunk)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->UpdateStream(aUpdateChunk);
}

NS_IMETHODIMP
nsUrlClassifierDBService::FinishStream()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->FinishStream();
}

NS_IMETHODIMP
nsUrlClassifierDBService::FinishUpdate()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  mInUpdate = false;

  return mWorkerProxy->FinishUpdate();
}


NS_IMETHODIMP
nsUrlClassifierDBService::CancelUpdate()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  mInUpdate = false;

  return mWorkerProxy->CancelUpdate();
}

NS_IMETHODIMP
nsUrlClassifierDBService::ResetDatabase()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->ResetDatabase();
}

nsresult
nsUrlClassifierDBService::CacheCompletions(CacheResultArray *results)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->CacheCompletions(results);
}

nsresult
nsUrlClassifierDBService::CacheMisses(PrefixArray *results)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->CacheMisses(results);
}

bool
nsUrlClassifierDBService::GetCompleter(const nsACString &tableName,
                                       nsIUrlClassifierHashCompleter **completer)
{
  
  
  if (mCompleters.Get(tableName, completer)) {
    return true;
  }

  
  
  if (!mGethashTables.Contains(tableName) ||
      mDisallowCompletionsTables.Contains(tableName)) {
    return false;
  }

  MOZ_ASSERT(!StringBeginsWith(tableName, NS_LITERAL_CSTRING("test-")),
             "We should never fetch hash completions for test tables");

  
  return NS_SUCCEEDED(CallGetService(NS_URLCLASSIFIERHASHCOMPLETER_CONTRACTID,
                                     completer));
}

NS_IMETHODIMP
nsUrlClassifierDBService::Observe(nsISupports *aSubject, const char *aTopic,
                                  const char16_t *aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> prefs(do_QueryInterface(aSubject, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    if (NS_LITERAL_STRING(CHECK_MALWARE_PREF).Equals(aData)) {
      mCheckMalware = Preferences::GetBool(CHECK_MALWARE_PREF,
        CHECK_MALWARE_DEFAULT);
    } else if (NS_LITERAL_STRING(CHECK_PHISHING_PREF).Equals(aData)) {
      mCheckPhishing = Preferences::GetBool(CHECK_PHISHING_PREF,
        CHECK_PHISHING_DEFAULT);
    } else if (NS_LITERAL_STRING(CHECK_TRACKING_PREF).Equals(aData) ||
               NS_LITERAL_STRING(CHECK_TRACKING_PB_PREF).Equals(aData)) {
      mCheckTracking =
        Preferences::GetBool(CHECK_TRACKING_PREF, CHECK_TRACKING_DEFAULT) ||
        Preferences::GetBool(CHECK_TRACKING_PB_PREF, CHECK_TRACKING_PB_DEFAULT);
    } else if (
      NS_LITERAL_STRING(PHISH_TABLE_PREF).Equals(aData) ||
      NS_LITERAL_STRING(MALWARE_TABLE_PREF).Equals(aData) ||
      NS_LITERAL_STRING(TRACKING_TABLE_PREF).Equals(aData) ||
      NS_LITERAL_STRING(DOWNLOAD_BLOCK_TABLE_PREF).Equals(aData) ||
      NS_LITERAL_STRING(DOWNLOAD_ALLOW_TABLE_PREF).Equals(aData) ||
      NS_LITERAL_STRING(DISALLOW_COMPLETION_TABLE_PREF).Equals(aData)) {
      
      ReadTablesFromPrefs();
    } else if (NS_LITERAL_STRING(CONFIRM_AGE_PREF).Equals(aData)) {
      gFreshnessGuarantee = Preferences::GetInt(CONFIRM_AGE_PREF,
        CONFIRM_AGE_DEFAULT_SEC);
    }
  } else if (!strcmp(aTopic, "profile-before-change") ||
             !strcmp(aTopic, "xpcom-shutdown-threads")) {
    Shutdown();
  } else {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


nsresult
nsUrlClassifierDBService::Shutdown()
{
  LOG(("shutting down db service\n"));

  if (!gDbBackgroundThread)
    return NS_OK;

  mCompleters.Clear();

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    prefs->RemoveObserver(CHECK_MALWARE_PREF, this);
    prefs->RemoveObserver(CHECK_PHISHING_PREF, this);
    prefs->RemoveObserver(CHECK_TRACKING_PREF, this);
    prefs->RemoveObserver(CHECK_TRACKING_PB_PREF, this);
    prefs->RemoveObserver(PHISH_TABLE_PREF, this);
    prefs->RemoveObserver(MALWARE_TABLE_PREF, this);
    prefs->RemoveObserver(TRACKING_TABLE_PREF, this);
    prefs->RemoveObserver(DOWNLOAD_BLOCK_TABLE_PREF, this);
    prefs->RemoveObserver(DOWNLOAD_ALLOW_TABLE_PREF, this);
    prefs->RemoveObserver(DISALLOW_COMPLETION_TABLE_PREF, this);
    prefs->RemoveObserver(CONFIRM_AGE_PREF, this);
  }

  DebugOnly<nsresult> rv;
  
  if (mWorker) {
    rv = mWorkerProxy->CancelUpdate();
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to post cancel update event");

    rv = mWorkerProxy->CloseDb();
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to post close db event");
  }

  mWorkerProxy = nullptr;

  LOG(("joining background thread"));

  gShuttingDownThread = true;

  nsIThread *backgroundThread = gDbBackgroundThread;
  gDbBackgroundThread = nullptr;
  backgroundThread->Shutdown();
  NS_RELEASE(backgroundThread);

  return NS_OK;
}

nsIThread*
nsUrlClassifierDBService::BackgroundThread()
{
  return gDbBackgroundThread;
}

