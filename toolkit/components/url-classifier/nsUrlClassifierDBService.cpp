









































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
#include "nsIPrefBranch2.h"
#include "nsIPrefService.h"
#include "nsIProperties.h"
#include "nsToolkitCompsCID.h"
#include "nsIUrlClassifierUtils.h"
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
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Telemetry.h"
#include "prlog.h"
#include "prprf.h"
#include "prnetdb.h"
#include "Entries.h"
#include "Classifier.h"
#include "ProtocolParser.h"

using namespace mozilla;
using namespace mozilla::safebrowsing;


#if defined(PR_LOGGING)
PRLogModuleInfo *gUrlClassifierDbServiceLog = nsnull;
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

#define GETHASH_NOISE_PREF      "urlclassifier.gethashnoise"
#define GETHASH_NOISE_DEFAULT   4

#define GETHASH_TABLES_PREF     "urlclassifier.gethashtables"

#define CONFIRM_AGE_PREF        "urlclassifier.confirm-age"
#define CONFIRM_AGE_DEFAULT_SEC (45 * 60)

class nsUrlClassifierDBServiceWorker;


static nsUrlClassifierDBService* sUrlClassifierDBService;

nsIThread* nsUrlClassifierDBService::gDbBackgroundThread = nsnull;



static bool gShuttingDownThread = false;

static PRInt32 gFreshnessGuarantee = CONFIRM_AGE_DEFAULT_SEC;

static void
SplitTables(const nsACString& str, nsTArray<nsCString>& tables)
{
  tables.Clear();

  nsACString::const_iterator begin, iter, end;
  str.BeginReading(begin);
  str.EndReading(end);
  while (begin != end) {
    iter = begin;
    FindCharInReadable(',', iter, end);
    tables.AppendElement(Substring(begin, iter));
    begin = iter;
    if (begin != end)
      begin++;
  }
}



class nsUrlClassifierDBServiceWorker : public nsIUrlClassifierDBServiceWorker
{
public:
  nsUrlClassifierDBServiceWorker();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERDBSERVICE
  NS_DECL_NSIURLCLASSIFIERDBSERVICEWORKER

  
  nsresult Init(PRInt32 gethashNoise, nsCOMPtr<nsIFile> aCacheDir);

  
  nsresult QueueLookup(const nsACString& lookupKey,
                       nsIUrlClassifierLookupCallback* callback);

  
  
  nsresult HandlePendingLookups();

private:
  
  ~nsUrlClassifierDBServiceWorker();

  
  nsUrlClassifierDBServiceWorker(nsUrlClassifierDBServiceWorker&);

  nsresult OpenDb();

  
  nsresult ApplyUpdate();

  
  void ResetStream();

 
 void ResetUpdate();

  
  nsresult DoLookup(const nsACString& spec, nsIUrlClassifierLookupCallback* c);

  
  nsresult AddNoise(const Prefix aPrefix,
                    const nsCString tableName,
                    PRInt32 aCount,
                    LookupResultArray& results);

  nsCOMPtr<nsICryptoHash> mCryptoHash;

  nsAutoPtr<Classifier> mClassifier;
  nsAutoPtr<ProtocolParser> mProtocolParser;

  
  nsCOMPtr<nsIFile> mCacheDir;

  
  
  nsTArray<TableUpdate*> mTableUpdates;

  PRInt32 mUpdateWait;

  
  
  PrefixArray mMissCache;

  nsresult mUpdateStatus;
  nsTArray<nsCString> mUpdateTables;

  nsCOMPtr<nsIUrlClassifierUpdateObserver> mUpdateObserver;
  bool mInStream;

  
  nsCString mUpdateClientKey;

  
  PRUint32 mHashKey;

  
  PRInt32 mGethashNoise;

  
  
  Mutex mPendingLookupLock;

  class PendingLookup {
  public:
    TimeStamp mStartTime;
    nsCString mKey;
    nsCOMPtr<nsIUrlClassifierLookupCallback> mCallback;
  };

  
  nsTArray<PendingLookup> mPendingLookups;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsUrlClassifierDBServiceWorker,
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
nsUrlClassifierDBServiceWorker::Init(PRInt32 gethashNoise,
                                     nsCOMPtr<nsIFile> aCacheDir)
{
  mGethashNoise = gethashNoise;
  mCacheDir = aCacheDir;

  ResetUpdate();

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::QueueLookup(const nsACString& spec,
                                            nsIUrlClassifierLookupCallback* callback)
{
  MutexAutoLock lock(mPendingLookupLock);

  PendingLookup* lookup = mPendingLookups.AppendElement();
  if (!lookup) return NS_ERROR_OUT_OF_MEMORY;

  lookup->mStartTime = TimeStamp::Now();
  lookup->mKey = spec;
  lookup->mCallback = callback;

  return NS_OK;
}












nsresult
nsUrlClassifierDBServiceWorker::DoLookup(const nsACString& spec,
                                         nsIUrlClassifierLookupCallback* c)
{
  if (gShuttingDownThread) {
    c->LookupComplete(nsnull);
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    c->LookupComplete(nsnull);
    return NS_ERROR_FAILURE;
  }

#if defined(PR_LOGGING)
  PRIntervalTime clockStart = 0;
  if (LOG_ENABLED()) {
    clockStart = PR_IntervalNow();
  }
#endif

  nsAutoPtr<LookupResultArray> results(new LookupResultArray());
  if (!results) {
    c->LookupComplete(nsnull);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  mClassifier->SetFreshTime(gFreshnessGuarantee);
  mClassifier->Check(spec, *results);

  LOG(("Found %d results.", results->Length()));

#if defined(PR_LOGGING)
  if (LOG_ENABLED()) {
    PRIntervalTime clockEnd = PR_IntervalNow();
    LOG(("query took %dms\n",
         PR_IntervalToMilliseconds(clockEnd - clockStart)));
  }
#endif

  nsAutoPtr<LookupResultArray> completes(new LookupResultArray());

  for (PRUint32 i = 0; i < results->Length(); i++) {
    if (!mMissCache.Contains(results->ElementAt(i).hash.prefix)) {
      completes->AppendElement(results->ElementAt(i));
    }
  }

  for (PRUint32 i = 0; i < completes->Length(); i++) {
    if (!completes->ElementAt(i).Confirmed()) {
      
      
      
      AddNoise(completes->ElementAt(i).mCodedPrefix,
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
      DoLookup(lookup.mKey, lookup.mCallback);
    }
    double lookupTime = (TimeStamp::Now() - lookup.mStartTime).ToMilliseconds();
    Telemetry::Accumulate(Telemetry::URLCLASSIFIER_LOOKUP_TIME,
                          static_cast<PRUint32>(lookupTime));
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::AddNoise(const Prefix aPrefix,
                                         const nsCString tableName,
                                         PRInt32 aCount,
                                         LookupResultArray& results)
{
  if (aCount < 1) {
    return NS_OK;
  }

  PrefixArray noiseEntries;
  nsresult rv = mClassifier->ReadNoiseEntries(aPrefix, tableName,
                                              aCount, &noiseEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < noiseEntries.Length(); i++) {
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
nsUrlClassifierDBServiceWorker::Lookup(const nsACString& spec,
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
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString response;
  mClassifier->TableRequest(response);
  c->HandleEvent(response);

  return rv;
}

void
nsUrlClassifierDBServiceWorker::ResetStream()
{
  LOG(("ResetStream"));
  mInStream = false;
  mProtocolParser = nsnull;
}

void
nsUrlClassifierDBServiceWorker::ResetUpdate()
{
  LOG(("ResetUpdate"));
  mUpdateWait = 0;
  mUpdateStatus = NS_OK;
  mUpdateObserver = nsnull;
  mUpdateClientKey.Truncate();
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::SetHashCompleter(const nsACString &tableName,
                                                 nsIUrlClassifierHashCompleter *completer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::BeginUpdate(nsIUrlClassifierUpdateObserver *observer,
                                            const nsACString &tables,
                                            const nsACString &clientKey)
{
  LOG(("nsUrlClassifierDBServiceWorker::BeginUpdate [%s]", PromiseFlatCString(tables).get()));

  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(!mUpdateObserver);

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  mUpdateStatus = NS_OK;
  mUpdateObserver = observer;
  SplitTables(tables, mUpdateTables);

  if (!clientKey.IsEmpty()) {
    rv = nsUrlClassifierUtils::DecodeClientKey(clientKey, mUpdateClientKey);
    NS_ENSURE_SUCCESS(rv, rv);
    LOG(("clientKey present, marking update key"));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::BeginStream(const nsACString &table,
                                            const nsACString &serverMAC)
{
  LOG(("nsUrlClassifierDBServiceWorker::BeginStream"));

  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(mUpdateObserver);
  NS_ENSURE_STATE(!mInStream);

  mInStream = true;

  NS_ASSERTION(!mProtocolParser, "Should not have a protocol parser.");

  mProtocolParser = new ProtocolParser(mHashKey);
  if (!mProtocolParser)
    return NS_ERROR_OUT_OF_MEMORY;

  mProtocolParser->Init(mCryptoHash);

  nsresult rv;

  
  if (!mUpdateClientKey.IsEmpty()) {
    LOG(("Expecting MAC in this stream"));
    rv = mProtocolParser->InitHMAC(mUpdateClientKey, serverMAC);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    LOG(("No MAC in this stream"));
  }

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
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(mInStream);
  NS_ENSURE_STATE(mUpdateObserver);

  mInStream = false;

  mProtocolParser->FinishHMAC();

  if (NS_SUCCEEDED(mProtocolParser->Status())) {
    if (mProtocolParser->UpdateWait()) {
      mUpdateWait = mProtocolParser->UpdateWait();
    }
    
    const nsTArray<ProtocolParser::ForwardedUpdate> &forwards =
      mProtocolParser->Forwards();
    for (uint32 i = 0; i < forwards.Length(); i++) {
      const ProtocolParser::ForwardedUpdate &forward = forwards[i];
      mUpdateObserver->UpdateUrlRequested(forward.url, forward.table, forward.mac);
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
  
  if (mProtocolParser->RekeyRequested()) {
    mUpdateObserver->RekeyRequested();
  }

  mProtocolParser = nsnull;
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
  mUpdateObserver = nsnull;

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
  NS_ENSURE_SUCCESS(rv, rv);

  mClassifier->Reset();

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
    mClassifier = nsnull;
  }

  mCryptoHash = nsnull;
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

  nsAutoPtr<ProtocolParser> pParse(new ProtocolParser(mHashKey));
  nsTArray<TableUpdate*> updates;

  
  
  
  nsTArray<nsCString> tables;
  nsresult rv = mClassifier->ActiveTables(tables);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < resultsPtr->Length(); i++) {
    bool activeTable = false;
    for (PRUint32 table = 0; table < tables.Length(); table++) {
      if (tables[table].Equals(resultsPtr->ElementAt(i).table)) {
        activeTable = true;
      }
    }
    if (activeTable) {
      TableUpdate * tu = pParse->GetTableUpdate(resultsPtr->ElementAt(i).table);
      LOG(("CacheCompletion Addchunk %d hash %X", resultsPtr->ElementAt(i).entry.addChunk,
           resultsPtr->ElementAt(i).entry.hash.prefix));
      tu->NewAddComplete(resultsPtr->ElementAt(i).entry.addChunk,
                         resultsPtr->ElementAt(i).entry.hash.complete);
      tu->NewAddChunk(resultsPtr->ElementAt(i).entry.addChunk);
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

  for (PRUint32 i = 0; i < resultsPtr->Length(); i++) {
    mMissCache.AppendElement(resultsPtr->ElementAt(i));
   }
  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::OpenDb()
{
  
  if (mClassifier) {
    return NS_OK;
  }

  LOG(("Opening db"));

  nsAutoPtr<Classifier> classifier(new Classifier());
  if (!classifier) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  classifier->SetFreshTime(gFreshnessGuarantee);

  nsresult rv = classifier->Open(*mCacheDir);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to open URL classifier.");
  }

  mHashKey = classifier->GetHashKey();
  mClassifier = classifier;

  mCryptoHash = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}








class nsUrlClassifierLookupCallback : public nsIUrlClassifierLookupCallback
                                    , public nsIUrlClassifierHashCompleterCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERLOOKUPCALLBACK
  NS_DECL_NSIURLCLASSIFIERHASHCOMPLETERCALLBACK

  nsUrlClassifierLookupCallback(nsUrlClassifierDBService *dbservice,
                                nsIUrlClassifierCallback *c)
    : mDBService(dbservice)
    , mResults(nsnull)
    , mPendingCompletions(0)
    , mCallback(c)
    {}

  ~nsUrlClassifierLookupCallback();

private:
  nsresult HandleResults();

  nsRefPtr<nsUrlClassifierDBService> mDBService;
  nsAutoPtr<LookupResultArray> mResults;

  
  nsAutoPtr<CacheResultArray> mCacheResults;

  PRUint32 mPendingCompletions;
  nsCOMPtr<nsIUrlClassifierCallback> mCallback;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsUrlClassifierLookupCallback,
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
  NS_ASSERTION(mResults == nsnull,
               "Should only get one set of results per nsUrlClassifierLookupCallback!");

  if (!results) {
    HandleResults();
    return NS_OK;
  }

  mResults = results;

  
  for (PRUint32 i = 0; i < results->Length(); i++) {
    LookupResult& result = results->ElementAt(i);

    
    if (!result.Confirmed()) {
      nsCOMPtr<nsIUrlClassifierHashCompleter> completer;
      if (mDBService->GetCompleter(result.mTableName,
                                   getter_AddRefs(completer))) {
        nsCAutoString partialHash;
        partialHash.Assign(reinterpret_cast<char*>(&result.hash.prefix),
                           PREFIX_SIZE);

        nsresult rv = completer->Complete(partialHash, this);
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
                                          PRUint32 chunkId,
                                          bool verified)
{
  LOG(("nsUrlClassifierLookupCallback::Completion [%p, %s, %d, %d]",
       this, PromiseFlatCString(tableName).get(), chunkId, verified));
  mozilla::safebrowsing::Completion hash;
  hash.Assign(completeHash);

  
  if (!mCacheResults) {
    mCacheResults = new CacheResultArray();
    if (!mCacheResults)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (verified) {
    CacheResult result;
    result.entry.addChunk = chunkId;
    result.entry.hash.complete = hash;
    result.table = tableName;

    
    mCacheResults->AppendElement(result);
  }

  
  for (PRUint32 i = 0; i < mResults->Length(); i++) {
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
  
  for (PRUint32 i = 0; i < mResults->Length(); i++) {
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
    for (uint32 i = 0; i < mResults->Length(); i++) {
      LookupResult &result = mResults->ElementAt(i);
      if (!result.Confirmed()) {
        cacheMisses->AppendElement(result.PrefixHash());
      }
    }
    
    mDBService->CacheMisses(cacheMisses.forget());
  }

  if (mCacheResults) {
    
    
    mDBService->CacheCompletions(mCacheResults.forget());
  }

  nsCAutoString tableStr;
  for (PRUint32 i = 0; i < tables.Length(); i++) {
    if (i != 0)
      tableStr.Append(',');
    tableStr.Append(tables[i]);
  }

  return mCallback->HandleEvent(tableStr);
}






class nsUrlClassifierClassifyCallback : public nsIUrlClassifierCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERCALLBACK

  nsUrlClassifierClassifyCallback(nsIURIClassifierCallback *c,
                                  bool checkMalware,
                                  bool checkPhishing)
    : mCallback(c)
    , mCheckMalware(checkMalware)
    , mCheckPhishing(checkPhishing)
    {}

private:
  nsCOMPtr<nsIURIClassifierCallback> mCallback;
  bool mCheckMalware;
  bool mCheckPhishing;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUrlClassifierClassifyCallback,
                              nsIUrlClassifierCallback)

NS_IMETHODIMP
nsUrlClassifierClassifyCallback::HandleEvent(const nsACString& tables)
{
  
  
  
  nsresult response = NS_OK;

  nsACString::const_iterator begin, end;

  tables.BeginReading(begin);
  tables.EndReading(end);
  if (mCheckMalware &&
      FindInReadable(NS_LITERAL_CSTRING("-malware-"), begin, end)) {
    response = NS_ERROR_MALWARE_URI;
  } else {
    
    tables.BeginReading(begin);

    if (mCheckPhishing &&
        FindInReadable(NS_LITERAL_CSTRING("-phish-"), begin, end)) {
      response = NS_ERROR_PHISHING_URI;
    }
  }

  mCallback->OnClassifyComplete(response);

  return NS_OK;
}





NS_IMPL_THREADSAFE_ISUPPORTS3(nsUrlClassifierDBService,
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
      return nsnull;
    }

    NS_ADDREF(sUrlClassifierDBService);   

    *result = sUrlClassifierDBService->Init();
    if (NS_FAILED(*result)) {
      NS_RELEASE(sUrlClassifierDBService);
      return nsnull;
    }
  } else {
    
    NS_ADDREF(sUrlClassifierDBService);   
  }
  return sUrlClassifierDBService;
}


nsUrlClassifierDBService::nsUrlClassifierDBService()
 : mCheckMalware(CHECK_MALWARE_DEFAULT)
 , mCheckPhishing(CHECK_PHISHING_DEFAULT)
 , mInUpdate(false)
{
}

nsUrlClassifierDBService::~nsUrlClassifierDBService()
{
  sUrlClassifierDBService = nsnull;
}

nsresult
nsUrlClassifierDBService::Init()
{
#if defined(PR_LOGGING)
  if (!gUrlClassifierDbServiceLog)
    gUrlClassifierDbServiceLog = PR_NewLogModule("UrlClassifierDbService");
#endif

  nsresult rv;

  
  nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);

  PRInt32 gethashNoise = 0;
  if (prefs) {
    bool tmpbool;
    rv = prefs->GetBoolPref(CHECK_MALWARE_PREF, &tmpbool);
    mCheckMalware = NS_SUCCEEDED(rv) ? tmpbool : CHECK_MALWARE_DEFAULT;

    prefs->AddObserver(CHECK_MALWARE_PREF, this, false);

    rv = prefs->GetBoolPref(CHECK_PHISHING_PREF, &tmpbool);
    mCheckPhishing = NS_SUCCEEDED(rv) ? tmpbool : CHECK_PHISHING_DEFAULT;

    prefs->AddObserver(CHECK_PHISHING_PREF, this, false);

    if (NS_FAILED(prefs->GetIntPref(GETHASH_NOISE_PREF, &gethashNoise))) {
      gethashNoise = GETHASH_NOISE_DEFAULT;
    }

    nsXPIDLCString tmpstr;
    if (NS_SUCCEEDED(prefs->GetCharPref(GETHASH_TABLES_PREF, getter_Copies(tmpstr)))) {
      SplitTables(tmpstr, mGethashWhitelist);
    }

    prefs->AddObserver(GETHASH_TABLES_PREF, this, false);

    PRInt32 tmpint;
    rv = prefs->GetIntPref(CONFIRM_AGE_PREF, &tmpint);
    PR_ATOMIC_SET(&gFreshnessGuarantee, NS_SUCCEEDED(rv) ? tmpint : CONFIRM_AGE_DEFAULT_SEC);

    prefs->AddObserver(CONFIRM_AGE_PREF, this, false);
  }

  
  nsCOMPtr<nsICryptoHash> acryptoHash = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> cacheDir;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                              getter_AddRefs(cacheDir));
  if (NS_FAILED(rv)) {
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(cacheDir));
  }

  
  rv = NS_NewThread(&gDbBackgroundThread);
  if (NS_FAILED(rv))
    return rv;

  mWorker = new nsUrlClassifierDBServiceWorker();
  if (!mWorker)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = mWorker->Init(gethashNoise, cacheDir);
  if (NS_FAILED(rv)) {
    mWorker = nsnull;
    return rv;
  }

  
  mWorkerProxy = new UrlClassifierDBServiceWorkerProxy(mWorker);

  mCompleters.Init();

  
  nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->AddObserver(this, "profile-before-change", false);
  observerService->AddObserver(this, "xpcom-shutdown-threads", false);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::Classify(nsIURI *uri,
                                   nsIURIClassifierCallback* c,
                                   bool* result)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  if (!(mCheckMalware || mCheckPhishing)) {
    *result = false;
    return NS_OK;
  }

  nsRefPtr<nsUrlClassifierClassifyCallback> callback =
    new nsUrlClassifierClassifyCallback(c, mCheckMalware, mCheckPhishing);
  if (!callback) return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = LookupURI(uri, callback, false, result);
  if (rv == NS_ERROR_MALFORMED_URI) {
    *result = false;
    
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::Lookup(const nsACString& spec,
                                 nsIUrlClassifierCallback* c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIURI> uri;

  nsresult rv = NS_NewURI(getter_AddRefs(uri), spec);
  NS_ENSURE_SUCCESS(rv, rv);

  uri = NS_GetInnermostURI(uri);
  if (!uri) {
    return NS_ERROR_FAILURE;
  }

  bool didLookup;
  return LookupURI(uri, c, true, &didLookup);
}

nsresult
nsUrlClassifierDBService::LookupURI(nsIURI* uri,
                                    nsIUrlClassifierCallback* c,
                                    bool forceLookup,
                                    bool *didLookup)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsCAutoString key;
  
  nsCOMPtr<nsIUrlClassifierUtils> utilsService =
    do_GetService(NS_URLCLASSIFIERUTILS_CONTRACTID);
  nsresult rv = utilsService->GetKeyForURI(uri, key);
  if (NS_FAILED(rv))
    return rv;

  if (forceLookup) {
    *didLookup = true;
  } else {
    bool clean = false;

    if (!clean) {
      nsCOMPtr<nsIPermissionManager> permissionManager =
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

      if (permissionManager) {
        PRUint32 perm;
        permissionManager->TestPermission(uri, "safe-browsing", &perm);
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

  
  
  rv = mWorker->QueueLookup(key, proxyCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  return mWorkerProxy->Lookup(EmptyCString(), nsnull);
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
    if (!mCompleters.Put(tableName, completer)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    mCompleters.Remove(tableName);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::BeginUpdate(nsIUrlClassifierUpdateObserver *observer,
                                      const nsACString &updateTables,
                                      const nsACString &clientKey)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  if (mInUpdate)
    return NS_ERROR_NOT_AVAILABLE;

  mInUpdate = true;

  
  nsCOMPtr<nsIUrlClassifierUpdateObserver> proxyObserver =
    new UrlClassifierUpdateObserverProxy(observer);

  return mWorkerProxy->BeginUpdate(proxyObserver, updateTables, clientKey);
}

NS_IMETHODIMP
nsUrlClassifierDBService::BeginStream(const nsACString &table,
                                      const nsACString &serverMAC)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->BeginStream(table, serverMAC);
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

  if (!mGethashWhitelist.Contains(tableName)) {
    return false;
  }

  return NS_SUCCEEDED(CallGetService(NS_URLCLASSIFIERHASHCOMPLETER_CONTRACTID,
                                     completer));
}

NS_IMETHODIMP
nsUrlClassifierDBService::Observe(nsISupports *aSubject, const char *aTopic,
                                  const PRUnichar *aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> prefs(do_QueryInterface(aSubject, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    if (NS_LITERAL_STRING(CHECK_MALWARE_PREF).Equals(aData)) {
      bool tmpbool;
      rv = prefs->GetBoolPref(CHECK_MALWARE_PREF, &tmpbool);
      mCheckMalware = NS_SUCCEEDED(rv) ? tmpbool : CHECK_MALWARE_DEFAULT;
    } else if (NS_LITERAL_STRING(CHECK_PHISHING_PREF).Equals(aData)) {
      bool tmpbool;
      rv = prefs->GetBoolPref(CHECK_PHISHING_PREF, &tmpbool);
      mCheckPhishing = NS_SUCCEEDED(rv) ? tmpbool : CHECK_PHISHING_DEFAULT;
    } else if (NS_LITERAL_STRING(GETHASH_TABLES_PREF).Equals(aData)) {
      mGethashWhitelist.Clear();
      nsXPIDLCString val;
      if (NS_SUCCEEDED(prefs->GetCharPref(GETHASH_TABLES_PREF, getter_Copies(val)))) {
        SplitTables(val, mGethashWhitelist);
      }
    } else if (NS_LITERAL_STRING(CONFIRM_AGE_PREF).Equals(aData)) {
      PRInt32 tmpint;
      rv = prefs->GetIntPref(CONFIRM_AGE_PREF, &tmpint);
      PR_ATOMIC_SET(&gFreshnessGuarantee, NS_SUCCEEDED(rv) ? tmpint : CONFIRM_AGE_DEFAULT_SEC);
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

  nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    prefs->RemoveObserver(CHECK_MALWARE_PREF, this);
    prefs->RemoveObserver(CHECK_PHISHING_PREF, this);
    prefs->RemoveObserver(GETHASH_TABLES_PREF, this);
    prefs->RemoveObserver(CONFIRM_AGE_PREF, this);
  }

  nsresult rv;
  
  if (mWorker) {
    rv = mWorkerProxy->CancelUpdate();
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to post cancel update event");

    rv = mWorkerProxy->CloseDb();
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to post close db event");
  }

  mWorkerProxy = nsnull;

  LOG(("joining background thread"));

  gShuttingDownThread = true;

  nsIThread *backgroundThread = gDbBackgroundThread;
  gDbBackgroundThread = nsnull;
  backgroundThread->Shutdown();
  NS_RELEASE(backgroundThread);

  return NS_OK;
}

nsIThread*
nsUrlClassifierDBService::BackgroundThread()
{
  return gDbBackgroundThread;
}

