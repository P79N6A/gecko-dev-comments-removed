




#include <algorithm>

#include "Seer.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsICancelable.h"
#include "nsIChannel.h"
#include "nsIDNSListener.h"
#include "nsIDNSService.h"
#include "nsIDocument.h"
#include "nsIFile.h"
#include "nsILoadContext.h"
#include "nsILoadGroup.h"
#include "nsINetworkSeerVerifier.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsISpeculativeConnect.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif
#include "prlog.h"

#include "mozIStorageConnection.h"
#include "mozIStorageService.h"
#include "mozIStorageStatement.h"
#include "mozStorageHelper.h"

#include "mozilla/Preferences.h"
#include "mozilla/storage.h"
#include "mozilla/Telemetry.h"

#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
#include "nsIPropertyBag2.h"
static const int32_t ANDROID_23_VERSION = 10;
#endif

using namespace mozilla;

namespace mozilla {
namespace net {

#define RETURN_IF_FAILED(_rv) \
  do { \
    if (NS_FAILED(_rv)) { \
      return; \
    } \
  } while (0)

const char SEER_ENABLED_PREF[] = "network.seer.enabled";
const char SEER_SSL_HOVER_PREF[] = "network.seer.enable-hover-on-ssl";

const char SEER_PAGE_DELTA_DAY_PREF[] = "network.seer.page-degradation.day";
const int SEER_PAGE_DELTA_DAY_DEFAULT = 0;
const char SEER_PAGE_DELTA_WEEK_PREF[] = "network.seer.page-degradation.week";
const int SEER_PAGE_DELTA_WEEK_DEFAULT = 5;
const char SEER_PAGE_DELTA_MONTH_PREF[] = "network.seer.page-degradation.month";
const int SEER_PAGE_DELTA_MONTH_DEFAULT = 10;
const char SEER_PAGE_DELTA_YEAR_PREF[] = "network.seer.page-degradation.year";
const int SEER_PAGE_DELTA_YEAR_DEFAULT = 25;
const char SEER_PAGE_DELTA_MAX_PREF[] = "network.seer.page-degradation.max";
const int SEER_PAGE_DELTA_MAX_DEFAULT = 50;
const char SEER_SUB_DELTA_DAY_PREF[] =
  "network.seer.subresource-degradation.day";
const int SEER_SUB_DELTA_DAY_DEFAULT = 1;
const char SEER_SUB_DELTA_WEEK_PREF[] =
  "network.seer.subresource-degradation.week";
const int SEER_SUB_DELTA_WEEK_DEFAULT = 10;
const char SEER_SUB_DELTA_MONTH_PREF[] =
  "network.seer.subresource-degradation.month";
const int SEER_SUB_DELTA_MONTH_DEFAULT = 25;
const char SEER_SUB_DELTA_YEAR_PREF[] =
  "network.seer.subresource-degradation.year";
const int SEER_SUB_DELTA_YEAR_DEFAULT = 50;
const char SEER_SUB_DELTA_MAX_PREF[] =
  "network.seer.subresource-degradation.max";
const int SEER_SUB_DELTA_MAX_DEFAULT = 100;

const char SEER_PRECONNECT_MIN_PREF[] =
  "network.seer.preconnect-min-confidence";
const int PRECONNECT_MIN_DEFAULT = 90;
const char SEER_PRERESOLVE_MIN_PREF[] =
  "network.seer.preresolve-min-confidence";
const int PRERESOLVE_MIN_DEFAULT = 60;
const char SEER_REDIRECT_LIKELY_PREF[] =
  "network.seer.redirect-likely-confidence";
const int REDIRECT_LIKELY_DEFAULT = 75;

const char SEER_MAX_QUEUE_SIZE_PREF[] = "network.seer.max-queue-size";
const uint32_t SEER_MAX_QUEUE_SIZE_DEFAULT = 50;


const long long ONE_DAY = 86400LL * 1000000LL;
const long long ONE_WEEK = 7LL * ONE_DAY;
const long long ONE_MONTH = 30LL * ONE_DAY;
const long long ONE_YEAR = 365LL * ONE_DAY;

const long STARTUP_WINDOW = 5L * 60L * 1000000L; 


static const int32_t SEER_SCHEMA_VERSION = 1;

struct SeerTelemetryAccumulators {
  Telemetry::AutoCounter<Telemetry::SEER_PREDICT_ATTEMPTS> mPredictAttempts;
  Telemetry::AutoCounter<Telemetry::SEER_LEARN_ATTEMPTS> mLearnAttempts;
  Telemetry::AutoCounter<Telemetry::SEER_PREDICT_FULL_QUEUE> mPredictFullQueue;
  Telemetry::AutoCounter<Telemetry::SEER_LEARN_FULL_QUEUE> mLearnFullQueue;
  Telemetry::AutoCounter<Telemetry::SEER_TOTAL_PREDICTIONS> mTotalPredictions;
  Telemetry::AutoCounter<Telemetry::SEER_TOTAL_PRECONNECTS> mTotalPreconnects;
  Telemetry::AutoCounter<Telemetry::SEER_TOTAL_PRERESOLVES> mTotalPreresolves;
  Telemetry::AutoCounter<Telemetry::SEER_PREDICTIONS_CALCULATED> mPredictionsCalculated;
};






class SeerDNSListener : public nsIDNSListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDNSLISTENER

  SeerDNSListener()
  { }

  virtual ~SeerDNSListener()
  { }
};

NS_IMPL_ISUPPORTS1(SeerDNSListener, nsIDNSListener);

NS_IMETHODIMP
SeerDNSListener::OnLookupComplete(nsICancelable *request,
                                  nsIDNSRecord *rec,
                                  nsresult status)
{
  return NS_OK;
}





static Seer *gSeer = nullptr;

#if defined(PR_LOGGING)
static PRLogModuleInfo *gSeerLog = nullptr;
#define SEER_LOG(args) PR_LOG(gSeerLog, 4, args)
#else
#define SEER_LOG(args)
#endif

NS_IMPL_ISUPPORTS4(Seer,
                   nsINetworkSeer,
                   nsIObserver,
                   nsISpeculativeConnectionOverrider,
                   nsIInterfaceRequestor)

Seer::Seer()
  :mInitialized(false)
  ,mEnabled(true)
  ,mEnableHoverOnSSL(false)
  ,mPageDegradationDay(SEER_PAGE_DELTA_DAY_DEFAULT)
  ,mPageDegradationWeek(SEER_PAGE_DELTA_WEEK_DEFAULT)
  ,mPageDegradationMonth(SEER_PAGE_DELTA_MONTH_DEFAULT)
  ,mPageDegradationYear(SEER_PAGE_DELTA_YEAR_DEFAULT)
  ,mPageDegradationMax(SEER_PAGE_DELTA_MAX_DEFAULT)
  ,mSubresourceDegradationDay(SEER_SUB_DELTA_DAY_DEFAULT)
  ,mSubresourceDegradationWeek(SEER_SUB_DELTA_WEEK_DEFAULT)
  ,mSubresourceDegradationMonth(SEER_SUB_DELTA_MONTH_DEFAULT)
  ,mSubresourceDegradationYear(SEER_SUB_DELTA_YEAR_DEFAULT)
  ,mSubresourceDegradationMax(SEER_SUB_DELTA_MAX_DEFAULT)
  ,mPreconnectMinConfidence(PRECONNECT_MIN_DEFAULT)
  ,mPreresolveMinConfidence(PRERESOLVE_MIN_DEFAULT)
  ,mRedirectLikelyConfidence(REDIRECT_LIKELY_DEFAULT)
  ,mMaxQueueSize(SEER_MAX_QUEUE_SIZE_DEFAULT)
  ,mStatements(mDB)
  ,mLastStartupTime(0)
  ,mStartupCount(0)
  ,mQueueSize(0)
  ,mQueueSizeLock("Seer.mQueueSizeLock")
{
#if defined(PR_LOGGING)
  gSeerLog = PR_NewLogModule("NetworkSeer");
#endif

  MOZ_ASSERT(!gSeer, "multiple Seer instances!");
  gSeer = this;
}

Seer::~Seer()
{
  if (mInitialized)
    Shutdown();

  RemoveObserver();

  gSeer = nullptr;
}



nsresult
Seer::InstallObserver()
{
  MOZ_ASSERT(NS_IsMainThread(), "Installing observer off main thread");

  nsresult rv = NS_OK;
  nsCOMPtr<nsIObserverService> obs =
    mozilla::services::GetObserverService();
  if (!obs) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  rv = obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (!prefs) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  Preferences::AddBoolVarCache(&mEnabled, SEER_ENABLED_PREF, true);
  Preferences::AddBoolVarCache(&mEnableHoverOnSSL, SEER_SSL_HOVER_PREF, false);
  Preferences::AddIntVarCache(&mPageDegradationDay, SEER_PAGE_DELTA_DAY_PREF,
                              SEER_PAGE_DELTA_DAY_DEFAULT);
  Preferences::AddIntVarCache(&mPageDegradationWeek, SEER_PAGE_DELTA_WEEK_PREF,
                              SEER_PAGE_DELTA_WEEK_DEFAULT);
  Preferences::AddIntVarCache(&mPageDegradationMonth,
                              SEER_PAGE_DELTA_MONTH_PREF,
                              SEER_PAGE_DELTA_MONTH_DEFAULT);
  Preferences::AddIntVarCache(&mPageDegradationYear, SEER_PAGE_DELTA_YEAR_PREF,
                              SEER_PAGE_DELTA_YEAR_DEFAULT);
  Preferences::AddIntVarCache(&mPageDegradationMax, SEER_PAGE_DELTA_MAX_PREF,
                              SEER_PAGE_DELTA_MAX_DEFAULT);

  Preferences::AddIntVarCache(&mSubresourceDegradationDay,
                              SEER_SUB_DELTA_DAY_PREF,
                              SEER_SUB_DELTA_DAY_DEFAULT);
  Preferences::AddIntVarCache(&mSubresourceDegradationWeek,
                              SEER_SUB_DELTA_WEEK_PREF,
                              SEER_SUB_DELTA_WEEK_DEFAULT);
  Preferences::AddIntVarCache(&mSubresourceDegradationMonth,
                              SEER_SUB_DELTA_MONTH_PREF,
                              SEER_SUB_DELTA_MONTH_DEFAULT);
  Preferences::AddIntVarCache(&mSubresourceDegradationYear,
                              SEER_SUB_DELTA_YEAR_PREF,
                              SEER_SUB_DELTA_YEAR_DEFAULT);
  Preferences::AddIntVarCache(&mSubresourceDegradationMax,
                              SEER_SUB_DELTA_MAX_PREF,
                              SEER_SUB_DELTA_MAX_DEFAULT);

  Preferences::AddIntVarCache(&mPreconnectMinConfidence,
                              SEER_PRECONNECT_MIN_PREF,
                              PRECONNECT_MIN_DEFAULT);
  Preferences::AddIntVarCache(&mPreresolveMinConfidence,
                              SEER_PRERESOLVE_MIN_PREF,
                              PRERESOLVE_MIN_DEFAULT);
  Preferences::AddIntVarCache(&mRedirectLikelyConfidence,
                              SEER_REDIRECT_LIKELY_PREF,
                              REDIRECT_LIKELY_DEFAULT);

  Preferences::AddIntVarCache(&mMaxQueueSize, SEER_MAX_QUEUE_SIZE_PREF,
                              SEER_MAX_QUEUE_SIZE_DEFAULT);

  return rv;
}

void
Seer::RemoveObserver()
{
  MOZ_ASSERT(NS_IsMainThread(), "Removing observer off main thread");

  nsCOMPtr<nsIObserverService> obs =
    mozilla::services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  }
}

NS_IMETHODIMP
Seer::Observe(nsISupports *subject, const char *topic,
              const PRUnichar *data_unicode)
{
  nsresult rv = NS_OK;

  if (!strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, topic)) {
    gSeer->Shutdown();
  }

  return rv;
}



NS_IMETHODIMP
Seer::GetIgnorePossibleSpdyConnections(bool *ignorePossibleSpdyConnections)
{
  *ignorePossibleSpdyConnections = true;
  return NS_OK;
}

NS_IMETHODIMP
Seer::GetParallelSpeculativeConnectLimit(
    uint32_t *parallelSpeculativeConnectLimit)
{
  *parallelSpeculativeConnectLimit = 6;
  return NS_OK;
}



NS_IMETHODIMP
Seer::GetInterface(const nsIID &iid, void **result)
{
  return QueryInterface(iid, result);
}

#ifdef MOZ_NUWA_PROCESS
class NuwaMarkSeerThreadRunner : public nsRunnable
{
  NS_IMETHODIMP Run() MOZ_OVERRIDE
  {
    if (IsNuwaProcess()) {
      NS_ASSERTION(NuwaMarkCurrentThread != nullptr,
                   "NuwaMarkCurrentThread is undefined!");
      NuwaMarkCurrentThread(nullptr, nullptr);
    }
    return NS_OK;
  }
};
#endif



nsresult
Seer::Init()
{
  if (!NS_IsMainThread()) {
    MOZ_ASSERT(false, "Seer::Init called off the main thread!");
    return NS_ERROR_UNEXPECTED;
  }

  nsresult rv = NS_OK;

#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
  
  
  
  nsCOMPtr<nsIPropertyBag2> infoService =
    do_GetService("@mozilla.org/system-info;1");
  if (infoService) {
    int32_t androidVersion = -1;
    rv = infoService->GetPropertyAsInt32(NS_LITERAL_STRING("version"),
                                         &androidVersion);
    if (NS_SUCCEEDED(rv) && (androidVersion < ANDROID_23_VERSION)) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }
#endif

  mStartupTime = PR_Now();

  mAccumulators = new SeerTelemetryAccumulators();

  rv = InstallObserver();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mDNSListener) {
    mDNSListener = new SeerDNSListener();
  }

  rv = NS_NewNamedThread("Network Seer", getter_AddRefs(mIOThread));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef MOZ_NUWA_PROCESS
  nsCOMPtr<NuwaMarkSeerThreadRunner> runner = new NuwaMarkSeerThreadRunner();
  mIOThread->Dispatch(runner, NS_DISPATCH_NORMAL);
#endif

  mSpeculativeService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mDnsService = do_GetService("@mozilla.org/network/dns-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mStorageService = do_GetService("@mozilla.org/storage/service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(mDBFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBFile->AppendNative(NS_LITERAL_CSTRING("seer.sqlite"));
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = true;

  return rv;
}





nsresult
Seer::EnsureInitStorage()
{
  MOZ_ASSERT(!NS_IsMainThread(), "Initializing seer storage on main thread");

  if (mDB) {
    return NS_OK;
  }

  nsresult rv;

  rv = mStorageService->OpenDatabase(mDBFile, getter_AddRefs(mDB));
  if (NS_FAILED(rv)) {
    
    
    rv = mDBFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStorageService->OpenDatabase(mDBFile, getter_AddRefs(mDB));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA synchronous = OFF;"));
  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA foreign_keys = ON;"));

  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_seer_version (\n"
                         "  version INTEGER NOT NULL\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mDB->CreateStatement(
      NS_LITERAL_CSTRING("SELECT version FROM moz_seer_version;\n"),
      getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasRows;
  rv = stmt->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasRows) {
    int32_t currentVersion;
    rv = stmt->GetInt32(0, &currentVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    MOZ_ASSERT(currentVersion == SEER_SCHEMA_VERSION,
               "Invalid seer schema version!");
    if (currentVersion != SEER_SCHEMA_VERSION) {
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    stmt = nullptr;
    rv = mDB->CreateStatement(
        NS_LITERAL_CSTRING("INSERT INTO moz_seer_version (version) VALUES "
                           "(:seer_version);"),
        getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("seer_version"),
                               SEER_SCHEMA_VERSION);
    NS_ENSURE_SUCCESS(rv, rv);

    stmt->Execute();
  }

  stmt = nullptr;

  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_hosts (\n"
                         "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                         "  origin TEXT NOT NULL,\n"
                         "  loads INTEGER DEFAULT 0,\n"
                         "  last_load INTEGER DEFAULT 0\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS host_id_origin_index "
                         "ON moz_hosts (id, origin);"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS host_origin_index "
                         "ON moz_hosts (origin);"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_subhosts (\n"
                         "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                         "  hid INTEGER NOT NULL,\n"
                         "  origin TEXT NOT NULL,\n"
                         "  hits INTEGER DEFAULT 0,\n"
                         "  last_hit INTEGER DEFAULT 0,\n"
                         "  FOREIGN KEY(hid) REFERENCES moz_hosts(id) ON DELETE CASCADE\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS subhost_hid_origin_index "
                         "ON moz_subhosts (hid, origin);"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS subhost_id_index "
                         "ON moz_subhosts (id);"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_startups (\n"
                         "  startups INTEGER,\n"
                         "  last_startup INTEGER\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->CreateStatement(
      NS_LITERAL_CSTRING("SELECT startups, last_startup FROM moz_startups;\n"),
      getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = stmt->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasRows) {
    
    stmt->GetInt32(0, &mStartupCount);
    stmt->GetInt64(1, &mLastStartupTime);

    
    stmt = nullptr;

    rv = mDB->CreateStatement(
        NS_LITERAL_CSTRING("UPDATE moz_startups SET startups = :startup_count, "
                           "last_startup = :startup_time;\n"),
        getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("startup_count"),
                               mStartupCount + 1);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("startup_time"),
                               mStartupTime);
    NS_ENSURE_SUCCESS(rv, rv);

    stmt->Execute();
  } else {
    
    mStartupCount = 1;

    rv = mDB->CreateStatement(
        NS_LITERAL_CSTRING("INSERT INTO moz_startups (startups, last_startup) "
                           "VALUES (1, :startup_time);\n"),
        getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("startup_time"),
                               mStartupTime);
    NS_ENSURE_SUCCESS(rv, rv);

    stmt->Execute();
  }

  
  stmt = nullptr;

  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_startup_pages (\n"
                         "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
                         "  uri TEXT NOT NULL,\n"
                         "  hits INTEGER DEFAULT 0,\n"
                         "  last_hit INTEGER DEFAULT 0\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS startup_page_uri_index "
                         "ON moz_startup_pages (uri);"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_pages (\n"
                         "  id integer PRIMARY KEY AUTOINCREMENT,\n"
                         "  uri TEXT NOT NULL,\n"
                         "  loads INTEGER DEFAULT 0,\n"
                         "  last_load INTEGER DEFAULT 0\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS page_id_uri_index "
                         "ON moz_pages (id, uri);"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS page_uri_index "
                         "ON moz_pages (uri);"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_subresources (\n"
                         "  id integer PRIMARY KEY AUTOINCREMENT,\n"
                         "  pid INTEGER NOT NULL,\n"
                         "  uri TEXT NOT NULL,\n"
                         "  hits INTEGER DEFAULT 0,\n"
                         "  last_hit INTEGER DEFAULT 0,\n"
                         "  FOREIGN KEY(pid) REFERENCES moz_pages(id) ON DELETE CASCADE\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS subresource_pid_uri_index "
                         "ON moz_subresources (pid, uri);"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS subresource_id_index "
                         "ON moz_subresources (id);"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_redirects (\n"
                         "  id integer PRIMARY KEY AUTOINCREMENT,\n"
                         "  pid integer NOT NULL,\n"
                         "  uri TEXT NOT NULL,\n"
                         "  origin TEXT NOT NULL,\n"
                         "  hits INTEGER DEFAULT 0,\n"
                         "  last_hit INTEGER DEFAULT 0,\n"
                         "  FOREIGN KEY(pid) REFERENCES moz_pages(id)\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS redirect_pid_uri_index "
                         "ON moz_redirects (pid, uri);"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS redirect_id_index "
                         "ON moz_redirects (id);"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

class SeerThreadShutdownRunner : public nsRunnable
{
public:
  SeerThreadShutdownRunner(nsIThread *ioThread)
    :mIOThread(ioThread)
  { }

  NS_IMETHODIMP Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread(), "Shut down seer io thread off main thread");
    mIOThread->Shutdown();
    return NS_OK;
  }

private:
  nsCOMPtr<nsIThread> mIOThread;
};

class SeerDBShutdownRunner : public nsRunnable
{
public:
  SeerDBShutdownRunner(nsIThread *ioThread, nsINetworkSeer *seer)
    :mIOThread(ioThread)
  {
    mSeer = new nsMainThreadPtrHolder<nsINetworkSeer>(seer);
  }

  NS_IMETHODIMP Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread(), "Shutting down DB on main thread");

    gSeer->mStatements.FinalizeStatements();
    gSeer->mDB->Close();
    gSeer->mDB = nullptr;

    nsRefPtr<SeerThreadShutdownRunner> runner =
      new SeerThreadShutdownRunner(mIOThread);
    NS_DispatchToMainThread(runner);

    return NS_OK;
  }

private:
  nsCOMPtr<nsIThread> mIOThread;

  
  nsMainThreadPtrHandle<nsINetworkSeer> mSeer;
};

void
Seer::Shutdown()
{
  if (!NS_IsMainThread()) {
    MOZ_ASSERT(false, "Seer::Shutdown called off the main thread!");
    return;
  }

  mInitialized = false;

  if (mIOThread) {
    nsCOMPtr<nsIThread> ioThread;
    mIOThread.swap(ioThread);

    if (mDB) {
      nsRefPtr<SeerDBShutdownRunner> runner =
        new SeerDBShutdownRunner(ioThread, this);
      ioThread->Dispatch(runner, NS_DISPATCH_NORMAL);
    } else {
      nsRefPtr<SeerThreadShutdownRunner> runner =
        new SeerThreadShutdownRunner(ioThread);
      NS_DispatchToMainThread(runner);
    }
  }
}

nsresult
Seer::Create(nsISupports *aOuter, const nsIID& aIID,
             void **aResult)
{
  nsresult rv;

  if (aOuter != nullptr) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsRefPtr<Seer> svc = new Seer();

  rv = svc->Init();
  if (NS_FAILED(rv)) {
    SEER_LOG(("Failed to initialize seer, seer will be a noop"));
  }

  
  
  
  rv = svc->QueryInterface(aIID, aResult);

  return rv;
}



static void
ExtractOrigin(nsIURI *uri, nsAutoCString &s)
{
  s.Truncate();

  nsAutoCString scheme;
  nsresult rv = uri->GetScheme(scheme);
  RETURN_IF_FAILED(rv);

  nsAutoCString host;
  rv = uri->GetAsciiHost(host);
  RETURN_IF_FAILED(rv);

  int32_t port;
  rv = uri->GetPort(&port);
  RETURN_IF_FAILED(rv);

  s.Assign(scheme);
  s.AppendLiteral("://");
  s.Append(host);
  if (port != -1) {
    s.AppendLiteral(":");
    s.AppendInt(port);
  }
}




class SeerPredictionEvent : public nsRunnable
{
public:
  SeerPredictionEvent(nsIURI *targetURI, nsIURI *sourceURI,
                      SeerPredictReason reason,
                      nsINetworkSeerVerifier *verifier)
    :mReason(reason)
  {
    MOZ_ASSERT(NS_IsMainThread(), "Creating prediction event off main thread");

    mEnqueueTime = TimeStamp::Now();

    if (verifier) {
      mVerifier = new nsMainThreadPtrHolder<nsINetworkSeerVerifier>(verifier);
    }
    if (targetURI) {
      targetURI->GetAsciiSpec(mTargetURI.spec);
      ExtractOrigin(targetURI, mTargetURI.origin);
    }
    if (sourceURI) {
      sourceURI->GetAsciiSpec(mSourceURI.spec);
      ExtractOrigin(sourceURI, mSourceURI.origin);
    }
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread(), "Running prediction event on main thread");

    Telemetry::AccumulateTimeDelta(Telemetry::SEER_PREDICT_WAIT_TIME,
                                   mEnqueueTime);

    TimeStamp startTime = TimeStamp::Now();

    nsresult rv = NS_OK;

    switch (mReason) {
      case nsINetworkSeer::PREDICT_LOAD:
        gSeer->PredictForPageload(mTargetURI, mVerifier, 0, mEnqueueTime);
        break;
      case nsINetworkSeer::PREDICT_STARTUP:
        gSeer->PredictForStartup(mVerifier, mEnqueueTime);
        break;
      default:
        MOZ_ASSERT(false, "Got unexpected value for predict reason");
        rv = NS_ERROR_UNEXPECTED;
    }

    gSeer->FreeSpaceInQueue();

    Telemetry::AccumulateTimeDelta(Telemetry::SEER_PREDICT_WORK_TIME,
                                   startTime);

    return rv;
  }

private:
  Seer::UriInfo mTargetURI;
  Seer::UriInfo mSourceURI;
  SeerPredictReason mReason;
  SeerVerifierHandle mVerifier;
  TimeStamp mEnqueueTime;
};




void
Seer::PredictForLink(nsIURI *targetURI, nsIURI *sourceURI,
                     nsINetworkSeerVerifier *verifier)
{
  MOZ_ASSERT(NS_IsMainThread(), "Predicting for link off main thread");

  if (!mSpeculativeService) {
    return;
  }

  if (!mEnableHoverOnSSL) {
    bool isSSL = false;
    sourceURI->SchemeIs("https", &isSSL);
    if (isSSL) {
      
      SEER_LOG(("Not predicting for link hover - on an SSL page"));
      return;
    }
  }

  mSpeculativeService->SpeculativeConnect(targetURI, nullptr);
  if (verifier) {
    verifier->OnPredictPreconnect(targetURI);
  }
}



class SeerPredictionRunner : public nsRunnable
{
public:
  SeerPredictionRunner(SeerVerifierHandle &verifier, TimeStamp predictStartTime)
    :mVerifier(verifier)
    ,mPredictStartTime(predictStartTime)
  { }

  void AddPreconnect(const nsACString &uri)
  {
    mPreconnects.AppendElement(uri);
  }

  void AddPreresolve(const nsACString &uri)
  {
    mPreresolves.AppendElement(uri);
  }

  bool HasWork() const
  {
    return !(mPreconnects.IsEmpty() && mPreresolves.IsEmpty());
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread(), "Running prediction off main thread");

    Telemetry::AccumulateTimeDelta(Telemetry::SEER_PREDICT_TIME_TO_ACTION,
                                   mPredictStartTime);

    uint32_t len, i;

    len = mPreconnects.Length();
    for (i = 0; i < len; ++i) {
      nsCOMPtr<nsIURI> uri;
      nsresult rv = NS_NewURI(getter_AddRefs(uri), mPreconnects[i]);
      if (NS_FAILED(rv)) {
        continue;
      }

      ++gSeer->mAccumulators->mTotalPredictions;
      ++gSeer->mAccumulators->mTotalPreconnects;
      gSeer->mSpeculativeService->SpeculativeConnect(uri, gSeer);
      if (mVerifier) {
        mVerifier->OnPredictPreconnect(uri);
      }
    }

    len = mPreresolves.Length();
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    for (i = 0; i < len; ++i) {
      nsCOMPtr<nsIURI> uri;
      nsresult rv = NS_NewURI(getter_AddRefs(uri), mPreresolves[i]);
      if (NS_FAILED(rv)) {
        continue;
      }

      ++gSeer->mAccumulators->mTotalPredictions;
      ++gSeer->mAccumulators->mTotalPreresolves;
      nsAutoCString hostname;
      uri->GetAsciiHost(hostname);
      nsCOMPtr<nsICancelable> tmpCancelable;
      gSeer->mDnsService->AsyncResolve(hostname,
                                       (nsIDNSService::RESOLVE_PRIORITY_MEDIUM |
                                        nsIDNSService::RESOLVE_SPECULATE),
                                       gSeer->mDNSListener, nullptr,
                                       getter_AddRefs(tmpCancelable));
      if (mVerifier) {
        mVerifier->OnPredictDNS(uri);
      }
    }

    mPreconnects.Clear();
    mPreresolves.Clear();

    return NS_OK;
  }

private:
  nsTArray<nsCString> mPreconnects;
  nsTArray<nsCString> mPreresolves;
  SeerVerifierHandle mVerifier;
  TimeStamp mPredictStartTime;
};






int
Seer::CalculateGlobalDegradation(PRTime now, PRTime lastLoad)
{
  int globalDegradation;
  PRTime delta = now - lastLoad;
  if (delta < ONE_DAY) {
    globalDegradation = mPageDegradationDay;
  } else if (delta < ONE_WEEK) {
    globalDegradation = mPageDegradationWeek;
  } else if (delta < ONE_MONTH) {
    globalDegradation = mPageDegradationMonth;
  } else if (delta < ONE_YEAR) {
    globalDegradation = mPageDegradationYear;
  } else {
    globalDegradation = mPageDegradationMax;
  }

  Telemetry::Accumulate(Telemetry::SEER_GLOBAL_DEGRADATION, globalDegradation);
  return globalDegradation;
}












int
Seer::CalculateConfidence(int baseConfidence, PRTime lastHit,
                          PRTime lastPossible, int globalDegradation)
{
  ++mAccumulators->mPredictionsCalculated;

  int maxConfidence = 100;
  int confidenceDegradation = 0;

  if (lastHit < lastPossible) {
    
    
    maxConfidence = mPreconnectMinConfidence - 1;

    
    
    
    PRTime delta = lastPossible - lastHit;
    if (delta == 0) {
      confidenceDegradation = 0;
    } else if (delta < ONE_DAY) {
      confidenceDegradation = mSubresourceDegradationDay;
    } else if (delta < ONE_WEEK) {
      confidenceDegradation = mSubresourceDegradationWeek;
    } else if (delta < ONE_MONTH) {
      confidenceDegradation = mSubresourceDegradationMonth;
    } else if (delta < ONE_YEAR) {
      confidenceDegradation = mSubresourceDegradationYear;
    } else {
      confidenceDegradation = mSubresourceDegradationMax;
      maxConfidence = 0;
    }
  }

  
  
  int confidence = baseConfidence - confidenceDegradation - globalDegradation;
  confidence = std::max(confidence, 0);
  confidence = std::min(confidence, maxConfidence);

  Telemetry::Accumulate(Telemetry::SEER_BASE_CONFIDENCE, baseConfidence);
  Telemetry::Accumulate(Telemetry::SEER_SUBRESOURCE_DEGRADATION,
                        confidenceDegradation);
  Telemetry::Accumulate(Telemetry::SEER_CONFIDENCE, confidence);
  return confidence;
}



void
Seer::SetupPrediction(int confidence, const nsACString &uri,
                      SeerPredictionRunner *runner)
{
    if (confidence >= mPreconnectMinConfidence) {
      runner->AddPreconnect(uri);
    } else if (confidence >= mPreresolveMinConfidence) {
      runner->AddPreresolve(uri);
    }
}




bool
Seer::LookupTopLevel(QueryType queryType, const nsACString &key,
                     TopLevelInfo &info)
{
  MOZ_ASSERT(!NS_IsMainThread(), "LookupTopLevel called on main thread.");

  nsCOMPtr<mozIStorageStatement> stmt;
  if (queryType == QUERY_PAGE) {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("SELECT id, loads, last_load FROM moz_pages WHERE "
                           "uri = :key;"));
  } else {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("SELECT id, loads, last_load FROM moz_hosts WHERE "
                           "origin = :key;"));
  }
  NS_ENSURE_TRUE(stmt, false);
  mozStorageStatementScoper scope(stmt);

  nsresult rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("key"), key);
  NS_ENSURE_SUCCESS(rv, false);

  bool hasRows;
  rv = stmt->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, false);

  if (!hasRows) {
    return false;
  }

  rv = stmt->GetInt32(0, &info.id);
  NS_ENSURE_SUCCESS(rv, false);

  rv = stmt->GetInt32(1, &info.loadCount);
  NS_ENSURE_SUCCESS(rv, false);

  rv = stmt->GetInt64(2, &info.lastLoad);
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}



void
Seer::AddTopLevel(QueryType queryType, const nsACString &key, PRTime now)
{
  MOZ_ASSERT(!NS_IsMainThread(), "AddTopLevel called on main thread.");

  nsCOMPtr<mozIStorageStatement> stmt;
  if (queryType == QUERY_PAGE) {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("INSERT INTO moz_pages (uri, loads, last_load) "
                           "VALUES (:key, 1, :now);"));
  } else {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("INSERT INTO moz_hosts (origin, loads, last_load) "
                           "VALUES (:key, 1, :now);"));
  }
  if (!stmt) {
    return;
  }
  mozStorageStatementScoper scope(stmt);

  
  
  nsresult rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("key"), key);
  RETURN_IF_FAILED(rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("now"), now);
  RETURN_IF_FAILED(rv);

  rv = stmt->Execute();
}



void
Seer::UpdateTopLevel(QueryType queryType, const TopLevelInfo &info, PRTime now)
{
  MOZ_ASSERT(!NS_IsMainThread(), "UpdateTopLevel called on main thread.");

  nsCOMPtr<mozIStorageStatement> stmt;
  if (queryType == QUERY_PAGE) {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("UPDATE moz_pages SET loads = :load_count, "
                           "last_load = :now WHERE id = :id;"));
  } else {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("UPDATE moz_hosts SET loads = :load_count, "
                           "last_load = :now WHERE id = :id;"));
  }
  if (!stmt) {
    return;
  }
  mozStorageStatementScoper scope(stmt);

  
  
  nsresult rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("load_count"),
                                      info.loadCount + 1);
  RETURN_IF_FAILED(rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("now"), now);
  RETURN_IF_FAILED(rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("id"), info.id);
  RETURN_IF_FAILED(rv);

  rv = stmt->Execute();
}






bool
Seer::TryPredict(QueryType queryType, const TopLevelInfo &info, PRTime now,
                 SeerVerifierHandle &verifier, TimeStamp &predictStartTime)
{
  MOZ_ASSERT(!NS_IsMainThread(), "TryPredict called on main thread.");

  int globalDegradation = CalculateGlobalDegradation(now, info.lastLoad);

  
  nsCOMPtr<mozIStorageStatement> stmt;
  if (queryType == QUERY_PAGE) {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("SELECT uri, hits, last_hit FROM moz_subresources "
                           "WHERE pid = :id;"));
  } else {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("SELECT origin, hits, last_hit FROM moz_subhosts "
                           "WHERE hid = :id;"));
  }
  NS_ENSURE_TRUE(stmt, false);
  mozStorageStatementScoper scope(stmt);

  nsresult rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("id"), info.id);
  NS_ENSURE_SUCCESS(rv, false);

  bool hasRows;
  rv = stmt->ExecuteStep(&hasRows);
  if (NS_FAILED(rv) || !hasRows) {
    return false;
  }

  nsRefPtr<SeerPredictionRunner> runner =
    new SeerPredictionRunner(verifier, predictStartTime);

  while (hasRows) {
    int32_t hitCount;
    PRTime lastHit;
    nsAutoCString subresource;
    int baseConfidence, confidence;

    
    
    
    

    rv = stmt->GetUTF8String(0, subresource);
    if NS_FAILED(rv) {
      goto nextrow;
    }

    rv = stmt->GetInt32(1, &hitCount);
    if (NS_FAILED(rv)) {
      goto nextrow;
    }

    rv = stmt->GetInt64(2, &lastHit);
    if (NS_FAILED(rv)) {
      goto nextrow;
    }

    baseConfidence = (hitCount * 100) / info.loadCount;
    confidence = CalculateConfidence(baseConfidence, lastHit, info.lastLoad,
                                     globalDegradation);
    SetupPrediction(confidence, subresource, runner);

nextrow:
    rv = stmt->ExecuteStep(&hasRows);
    NS_ENSURE_SUCCESS(rv, false);
  }

  bool predicted = false;

  if (runner->HasWork()) {
    NS_DispatchToMainThread(runner);
    predicted = true;
  }

  return predicted;
}


bool
Seer::WouldRedirect(const TopLevelInfo &info, PRTime now, UriInfo &newUri)
{
  MOZ_ASSERT(!NS_IsMainThread(), "WouldRedirect called on main thread.");

  nsCOMPtr<mozIStorageStatement> stmt = mStatements.GetCachedStatement(
      NS_LITERAL_CSTRING("SELECT uri, origin, hits, last_hit "
                         "FROM moz_redirects WHERE pid = :id;"));
  NS_ENSURE_TRUE(stmt, false);
  mozStorageStatementScoper scope(stmt);

  nsresult rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("id"), info.id);
  NS_ENSURE_SUCCESS(rv, false);

  bool hasRows;
  rv = stmt->ExecuteStep(&hasRows);
  if (NS_FAILED(rv) || !hasRows) {
    return false;
  }

  rv = stmt->GetUTF8String(0, newUri.spec);
  NS_ENSURE_SUCCESS(rv, false);

  rv = stmt->GetUTF8String(1, newUri.origin);
  NS_ENSURE_SUCCESS(rv, false);

  int32_t hitCount;
  rv = stmt->GetInt32(2, &hitCount);
  NS_ENSURE_SUCCESS(rv, false);

  PRTime lastHit;
  rv = stmt->GetInt64(3, &lastHit);
  NS_ENSURE_SUCCESS(rv, false);

  int globalDegradation = CalculateGlobalDegradation(now, info.lastLoad);
  int baseConfidence = (hitCount * 100) / info.loadCount;
  int confidence = CalculateConfidence(baseConfidence, lastHit, info.lastLoad,
                                       globalDegradation);

  if (confidence > mRedirectLikelyConfidence) {
    return true;
  }

  return false;
}



void
Seer::MaybeLearnForStartup(const UriInfo &uri, const PRTime now)
{
  MOZ_ASSERT(!NS_IsMainThread(), "MaybeLearnForStartup called on main thread.");

  if ((now - mStartupTime) < STARTUP_WINDOW) {
    LearnForStartup(uri);
  }
}

const int MAX_PAGELOAD_DEPTH = 10;


void
Seer::PredictForPageload(const UriInfo &uri, SeerVerifierHandle &verifier,
                         int stackCount, TimeStamp &predictStartTime)
{
  MOZ_ASSERT(!NS_IsMainThread(), "PredictForPageload called on main thread.");

  if (stackCount > MAX_PAGELOAD_DEPTH) {
    SEER_LOG(("Too deep into pageload prediction"));
    return;
  }

  if (NS_FAILED(EnsureInitStorage())) {
    return;
  }

  PRTime now = PR_Now();

  MaybeLearnForStartup(uri, now);

  TopLevelInfo pageInfo;
  TopLevelInfo originInfo;
  bool havePage = LookupTopLevel(QUERY_PAGE, uri.spec, pageInfo);
  bool haveOrigin = LookupTopLevel(QUERY_ORIGIN, uri.origin, originInfo);

  if (!havePage) {
    AddTopLevel(QUERY_PAGE, uri.spec, now);
  } else {
    UpdateTopLevel(QUERY_PAGE, pageInfo, now);
  }

  if (!haveOrigin) {
    AddTopLevel(QUERY_ORIGIN, uri.origin, now);
  } else {
    UpdateTopLevel(QUERY_ORIGIN, originInfo, now);
  }

  UriInfo newUri;
  if (havePage && WouldRedirect(pageInfo, now, newUri)) {
    nsRefPtr<SeerPredictionRunner> runner =
      new SeerPredictionRunner(verifier, predictStartTime);
    runner->AddPreconnect(newUri.spec);
    NS_DispatchToMainThread(runner);
    PredictForPageload(newUri, verifier, stackCount + 1, predictStartTime);
    return;
  }

  bool predicted = false;

  
  
  if (havePage) {
    predicted = TryPredict(QUERY_PAGE, pageInfo, now, verifier,
                           predictStartTime);
  }

  if (!predicted && haveOrigin) {
    predicted = TryPredict(QUERY_ORIGIN, originInfo, now, verifier,
                           predictStartTime);
  }

  if (!predicted) {
    Telemetry::AccumulateTimeDelta(Telemetry::SEER_PREDICT_TIME_TO_INACTION,
                                   predictStartTime);
  }
}



void
Seer::PredictForStartup(SeerVerifierHandle &verifier,
                        TimeStamp &predictStartTime)
{
  MOZ_ASSERT(!NS_IsMainThread(), "PredictForStartup called on main thread");

  if (NS_FAILED(EnsureInitStorage())) {
    return;
  }

  nsCOMPtr<mozIStorageStatement> stmt = mStatements.GetCachedStatement(
      NS_LITERAL_CSTRING("SELECT uri, hits, last_hit FROM moz_startup_pages;"));
  if (!stmt) {
    return;
  }
  mozStorageStatementScoper scope(stmt);
  nsresult rv;
  bool hasRows;

  nsRefPtr<SeerPredictionRunner> runner =
    new SeerPredictionRunner(verifier, predictStartTime);

  rv = stmt->ExecuteStep(&hasRows);
  RETURN_IF_FAILED(rv);

  while (hasRows) {
    nsAutoCString uri;
    int32_t hitCount;
    PRTime lastHit;
    int baseConfidence, confidence;

    
    
    
    

    rv = stmt->GetUTF8String(0, uri);
    if (NS_FAILED(rv)) {
      goto nextrow;
    }

    rv = stmt->GetInt32(1, &hitCount);
    if (NS_FAILED(rv)) {
      goto nextrow;
    }

    rv = stmt->GetInt64(2, &lastHit);
    if (NS_FAILED(rv)) {
      goto nextrow;
    }

    baseConfidence = (hitCount * 100) / mStartupCount;
    confidence = CalculateConfidence(baseConfidence, lastHit,
                                     mLastStartupTime, 0);
    SetupPrediction(confidence, uri, runner);

nextrow:
    rv = stmt->ExecuteStep(&hasRows);
    RETURN_IF_FAILED(rv);
  }

  if (runner->HasWork()) {
    NS_DispatchToMainThread(runner);
  } else {
    Telemetry::AccumulateTimeDelta(Telemetry::SEER_PREDICT_TIME_TO_INACTION,
                                   predictStartTime);
  }
}



static bool
IsNullOrHttp(nsIURI *uri)
{
  if (!uri) {
    return true;
  }

  bool isHTTP = false;
  uri->SchemeIs("http", &isHTTP);
  if (!isHTTP) {
    uri->SchemeIs("https", &isHTTP);
  }

  return isHTTP;
}

nsresult
Seer::ReserveSpaceInQueue()
{
  MutexAutoLock lock(mQueueSizeLock);

  if (mQueueSize >= mMaxQueueSize) {
    SEER_LOG(("Not enqueuing event - queue too large"));
    return NS_ERROR_NOT_AVAILABLE;
  }

  mQueueSize++;
  return NS_OK;
}

void
Seer::FreeSpaceInQueue()
{
  MutexAutoLock lock(mQueueSizeLock);
  MOZ_ASSERT(mQueueSize > 0, "unexpected mQueueSize");
  mQueueSize--;
}


NS_IMETHODIMP
Seer::Predict(nsIURI *targetURI, nsIURI *sourceURI, SeerPredictReason reason,
              nsILoadContext *loadContext, nsINetworkSeerVerifier *verifier)
{
  MOZ_ASSERT(NS_IsMainThread(),
             "Seer interface methods must be called on the main thread");

  if (!mInitialized) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!mEnabled) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (loadContext && loadContext->UsePrivateBrowsing()) {
    
    return NS_OK;
  }

  if (!IsNullOrHttp(targetURI) || !IsNullOrHttp(sourceURI)) {
    
    return NS_OK;
  }

  
  
  switch (reason) {
    case nsINetworkSeer::PREDICT_LINK:
      if (!targetURI || !sourceURI) {
        return NS_ERROR_INVALID_ARG;
      }
      
      
      PredictForLink(targetURI, sourceURI, verifier);
      return NS_OK;
    case nsINetworkSeer::PREDICT_LOAD:
      if (!targetURI || sourceURI) {
        return NS_ERROR_INVALID_ARG;
      }
      break;
    case nsINetworkSeer::PREDICT_STARTUP:
      if (targetURI || sourceURI) {
        return NS_ERROR_INVALID_ARG;
      }
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }

  ++mAccumulators->mPredictAttempts;
  nsresult rv = ReserveSpaceInQueue();
  if (NS_FAILED(rv)) {
    ++mAccumulators->mPredictFullQueue;
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<SeerPredictionEvent> event = new SeerPredictionEvent(targetURI,
                                                                sourceURI,
                                                                reason,
                                                                verifier);
  return mIOThread->Dispatch(event, NS_DISPATCH_NORMAL);
}



class SeerLearnEvent : public nsRunnable
{
public:
  SeerLearnEvent(nsIURI *targetURI, nsIURI *sourceURI, SeerLearnReason reason)
    :mReason(reason)
  {
    MOZ_ASSERT(NS_IsMainThread(), "Creating learn event off main thread");

    mEnqueueTime = TimeStamp::Now();

    targetURI->GetAsciiSpec(mTargetURI.spec);
    ExtractOrigin(targetURI, mTargetURI.origin);
    if (sourceURI) {
      sourceURI->GetAsciiSpec(mSourceURI.spec);
      ExtractOrigin(sourceURI, mSourceURI.origin);
    }
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread(), "Running learn off main thread");

    nsresult rv = NS_OK;

    Telemetry::AccumulateTimeDelta(Telemetry::SEER_LEARN_WAIT_TIME,
                                   mEnqueueTime);

    TimeStamp startTime = TimeStamp::Now();

    switch (mReason) {
    case nsINetworkSeer::LEARN_LOAD_TOPLEVEL:
      gSeer->LearnForToplevel(mTargetURI);
      break;
    case nsINetworkSeer::LEARN_LOAD_REDIRECT:
      gSeer->LearnForRedirect(mTargetURI, mSourceURI);
      break;
    case nsINetworkSeer::LEARN_LOAD_SUBRESOURCE:
      gSeer->LearnForSubresource(mTargetURI, mSourceURI);
      break;
    case nsINetworkSeer::LEARN_STARTUP:
      gSeer->LearnForStartup(mTargetURI);
      break;
    default:
      MOZ_ASSERT(false, "Got unexpected value for learn reason");
      rv = NS_ERROR_UNEXPECTED;
    }

    gSeer->FreeSpaceInQueue();

    Telemetry::AccumulateTimeDelta(Telemetry::SEER_LEARN_WORK_TIME, startTime);

    return rv;
  }
private:
  Seer::UriInfo mTargetURI;
  Seer::UriInfo mSourceURI;
  SeerLearnReason mReason;
  TimeStamp mEnqueueTime;
};

void
Seer::LearnForToplevel(const UriInfo &uri)
{
  MOZ_ASSERT(!NS_IsMainThread(), "LearnForToplevel called on main thread.");

  if (NS_FAILED(EnsureInitStorage())) {
    return;
  }

  PRTime now = PR_Now();

  MaybeLearnForStartup(uri, now);

  TopLevelInfo pageInfo;
  TopLevelInfo originInfo;
  bool havePage = LookupTopLevel(QUERY_PAGE, uri.spec, pageInfo);
  bool haveOrigin = LookupTopLevel(QUERY_ORIGIN, uri.origin, originInfo);

  if (!havePage) {
    AddTopLevel(QUERY_PAGE, uri.spec, now);
  } else {
    UpdateTopLevel(QUERY_PAGE, pageInfo, now);
  }

  if (!haveOrigin) {
    AddTopLevel(QUERY_ORIGIN, uri.origin, now);
  } else {
    UpdateTopLevel(QUERY_ORIGIN, originInfo, now);
  }
}



bool
Seer::LookupSubresource(QueryType queryType, const int32_t parentId,
                        const nsACString &key, SubresourceInfo &info)
{
  MOZ_ASSERT(!NS_IsMainThread(), "LookupSubresource called on main thread.");

  nsCOMPtr<mozIStorageStatement> stmt;
  if (queryType == QUERY_PAGE) {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("SELECT id, hits, last_hit FROM moz_subresources "
                           "WHERE pid = :parent_id AND uri = :key;"));
  } else {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("SELECT id, hits, last_hit FROM moz_subhosts WHERE "
                           "hid = :parent_id AND origin = :key;"));
  }
  NS_ENSURE_TRUE(stmt, false);
  mozStorageStatementScoper scope(stmt);

  nsresult rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("parent_id"),
                                      parentId);
  NS_ENSURE_SUCCESS(rv, false);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("key"), key);
  NS_ENSURE_SUCCESS(rv, false);

  bool hasRows;
  rv = stmt->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, false);
  if (!hasRows) {
    return false;
  }

  rv = stmt->GetInt32(0, &info.id);
  NS_ENSURE_SUCCESS(rv, false);

  rv = stmt->GetInt32(1, &info.hitCount);
  NS_ENSURE_SUCCESS(rv, false);

  rv = stmt->GetInt64(2, &info.lastHit);
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}


void
Seer::AddSubresource(QueryType queryType, const int32_t parentId,
                     const nsACString &key, const PRTime now)
{
  MOZ_ASSERT(!NS_IsMainThread(), "AddSubresource called on main thread.");

  nsCOMPtr<mozIStorageStatement> stmt;
  if (queryType == QUERY_PAGE) {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("INSERT INTO moz_subresources "
                           "(pid, uri, hits, last_hit) VALUES "
                           "(:parent_id, :key, 1, :now);"));
  } else {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("INSERT INTO moz_subhosts "
                           "(hid, origin, hits, last_hit) VALUES "
                           "(:parent_id, :key, 1, :now);"));
  }
  if (!stmt) {
    return;
  }
  mozStorageStatementScoper scope(stmt);

  nsresult rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("parent_id"),
                                      parentId);
  RETURN_IF_FAILED(rv);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("key"), key);
  RETURN_IF_FAILED(rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("now"), now);
  RETURN_IF_FAILED(rv);

  rv = stmt->Execute();
}



void
Seer::UpdateSubresource(QueryType queryType, const SubresourceInfo &info,
                        const PRTime now)
{
  MOZ_ASSERT(!NS_IsMainThread(), "UpdateSubresource called on main thread.");

  nsCOMPtr<mozIStorageStatement> stmt;
  if (queryType == QUERY_PAGE) {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("UPDATE moz_subresources SET hits = :hit_count, "
                           "last_hit = :now WHERE id = :id;"));
  } else {
    stmt = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("UPDATE moz_subhosts SET hits = :hit_count, "
                           "last_hit = :now WHERE id = :id;"));
  }
  if (!stmt) {
    return;
  }
  mozStorageStatementScoper scope(stmt);

  nsresult rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hit_count"),
                                      info.hitCount + 1);
  RETURN_IF_FAILED(rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("now"), now);
  RETURN_IF_FAILED(rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("id"), info.id);
  RETURN_IF_FAILED(rv);

  rv = stmt->Execute();
}



void
Seer::LearnForSubresource(const UriInfo &targetURI, const UriInfo &sourceURI)
{
  MOZ_ASSERT(!NS_IsMainThread(), "LearnForSubresource called on main thread.");

  if (NS_FAILED(EnsureInitStorage())) {
    return;
  }

  TopLevelInfo pageInfo, originInfo;
  bool havePage = LookupTopLevel(QUERY_PAGE, sourceURI.spec, pageInfo);
  bool haveOrigin = LookupTopLevel(QUERY_ORIGIN, sourceURI.origin,
                                   originInfo);

  if (!havePage && !haveOrigin) {
    
    return;
  }

  SubresourceInfo resourceInfo;
  bool haveResource = false;
  if (havePage) {
    haveResource = LookupSubresource(QUERY_PAGE, pageInfo.id, targetURI.spec,
                                     resourceInfo);
  }

  SubresourceInfo hostInfo;
  bool haveHost = false;
  if (haveOrigin) {
    haveHost = LookupSubresource(QUERY_ORIGIN, originInfo.id, targetURI.origin,
                                 hostInfo);
  }

  PRTime now = PR_Now();

  if (haveResource) {
    UpdateSubresource(QUERY_PAGE, resourceInfo, now);
  } else if (havePage) {
    AddSubresource(QUERY_PAGE, pageInfo.id, targetURI.spec, now);
  }
  

  if (haveHost) {
    UpdateSubresource(QUERY_ORIGIN, hostInfo, now);
  } else if (haveOrigin) {
    AddSubresource(QUERY_ORIGIN, originInfo.id, targetURI.origin, now);
  }
  
}



void
Seer::LearnForRedirect(const UriInfo &targetURI, const UriInfo &sourceURI)
{
  MOZ_ASSERT(!NS_IsMainThread(), "LearnForRedirect called on main thread.");

  if (NS_FAILED(EnsureInitStorage())) {
    return;
  }

  PRTime now = PR_Now();
  nsresult rv;

  nsCOMPtr<mozIStorageStatement> getPage = mStatements.GetCachedStatement(
      NS_LITERAL_CSTRING("SELECT id FROM moz_pages WHERE uri = :spec;"));
  if (!getPage) {
    return;
  }
  mozStorageStatementScoper scopedPage(getPage);

  
  rv = getPage->BindUTF8StringByName(NS_LITERAL_CSTRING("spec"),
                                     sourceURI.spec);
  RETURN_IF_FAILED(rv);

  bool hasRows;
  rv = getPage->ExecuteStep(&hasRows);
  if (NS_FAILED(rv) || !hasRows) {
    return;
  }

  int32_t pageId;
  rv = getPage->GetInt32(0, &pageId);
  RETURN_IF_FAILED(rv);

  nsCOMPtr<mozIStorageStatement> getRedirect = mStatements.GetCachedStatement(
      NS_LITERAL_CSTRING("SELECT id, hits FROM moz_redirects WHERE "
                         "pid = :page_id AND uri = :spec;"));
  if (!getRedirect) {
    return;
  }
  mozStorageStatementScoper scopedRedirect(getRedirect);

  rv = getRedirect->BindInt32ByName(NS_LITERAL_CSTRING("page_id"), pageId);
  RETURN_IF_FAILED(rv);

  rv = getRedirect->BindUTF8StringByName(NS_LITERAL_CSTRING("spec"),
                                         targetURI.spec);
  RETURN_IF_FAILED(rv);

  rv = getRedirect->ExecuteStep(&hasRows);
  RETURN_IF_FAILED(rv);

  if (!hasRows) {
    
    nsCOMPtr<mozIStorageStatement> addRedirect = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("INSERT INTO moz_redirects "
                           "(pid, uri, origin, hits, last_hit) VALUES "
                           "(:page_id, :spec, :origin, 1, :now);"));
    if (!addRedirect) {
      return;
    }
    mozStorageStatementScoper scopedAdd(addRedirect);

    rv = addRedirect->BindInt32ByName(NS_LITERAL_CSTRING("page_id"), pageId);
    RETURN_IF_FAILED(rv);

    rv = addRedirect->BindUTF8StringByName(NS_LITERAL_CSTRING("spec"),
                                           targetURI.spec);
    RETURN_IF_FAILED(rv);

    rv = addRedirect->BindUTF8StringByName(NS_LITERAL_CSTRING("origin"),
                                           targetURI.origin);
    RETURN_IF_FAILED(rv);

    rv = addRedirect->BindInt64ByName(NS_LITERAL_CSTRING("now"), now);
    RETURN_IF_FAILED(rv);

    rv = addRedirect->Execute();
  } else {
    
    int32_t redirId, hits;
    rv = getRedirect->GetInt32(0, &redirId);
    RETURN_IF_FAILED(rv);

    rv = getRedirect->GetInt32(1, &hits);
    RETURN_IF_FAILED(rv);

    nsCOMPtr<mozIStorageStatement> updateRedirect =
      mStatements.GetCachedStatement(
          NS_LITERAL_CSTRING("UPDATE moz_redirects SET hits = :hits, "
                             "last_hit = :now WHERE id = :redir;"));
    if (!updateRedirect) {
      return;
    }
    mozStorageStatementScoper scopedUpdate(updateRedirect);

    rv = updateRedirect->BindInt32ByName(NS_LITERAL_CSTRING("hits"), hits + 1);
    RETURN_IF_FAILED(rv);

    rv = updateRedirect->BindInt64ByName(NS_LITERAL_CSTRING("now"), now);
    RETURN_IF_FAILED(rv);

    rv = updateRedirect->BindInt32ByName(NS_LITERAL_CSTRING("redir"), redirId);
    RETURN_IF_FAILED(rv);

    updateRedirect->Execute();
  }
}


void
Seer::LearnForStartup(const UriInfo &uri)
{
  MOZ_ASSERT(!NS_IsMainThread(), "LearnForStartup called on main thread.");

  if (NS_FAILED(EnsureInitStorage())) {
    return;
  }

  nsCOMPtr<mozIStorageStatement> getPage = mStatements.GetCachedStatement(
      NS_LITERAL_CSTRING("SELECT id, hits FROM moz_startup_pages WHERE "
                         "uri = :origin;"));
  if (!getPage) {
    return;
  }
  mozStorageStatementScoper scopedPage(getPage);
  nsresult rv;

  rv = getPage->BindUTF8StringByName(NS_LITERAL_CSTRING("origin"), uri.origin);
  RETURN_IF_FAILED(rv);

  bool hasRows;
  rv = getPage->ExecuteStep(&hasRows);
  RETURN_IF_FAILED(rv);

  if (hasRows) {
    
    int32_t pageId, hitCount;

    rv = getPage->GetInt32(0, &pageId);
    RETURN_IF_FAILED(rv);

    rv = getPage->GetInt32(1, &hitCount);
    RETURN_IF_FAILED(rv);

    nsCOMPtr<mozIStorageStatement> updatePage = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("UPDATE moz_startup_pages SET hits = :hit_count, "
                           "last_hit = :startup_time WHERE id = :page_id;"));
    if (!updatePage) {
      return;
    }
    mozStorageStatementScoper scopedUpdate(updatePage);

    rv = updatePage->BindInt32ByName(NS_LITERAL_CSTRING("hit_count"),
                                     hitCount + 1);
    RETURN_IF_FAILED(rv);

    rv = updatePage->BindInt64ByName(NS_LITERAL_CSTRING("startup_time"),
                                     mStartupTime);
    RETURN_IF_FAILED(rv);

    rv = updatePage->BindInt32ByName(NS_LITERAL_CSTRING("page_id"), pageId);
    RETURN_IF_FAILED(rv);

    updatePage->Execute();
  } else {
    
    nsCOMPtr<mozIStorageStatement> addPage = mStatements.GetCachedStatement(
        NS_LITERAL_CSTRING("INSERT INTO moz_startup_pages (uri, hits, "
                           "last_hit) VALUES (:origin, 1, :startup_time);"));
    if (!addPage) {
      return;
    }
    mozStorageStatementScoper scopedAdd(addPage);
    rv = addPage->BindUTF8StringByName(NS_LITERAL_CSTRING("origin"),
                                       uri.origin);
    RETURN_IF_FAILED(rv);

    rv = addPage->BindInt64ByName(NS_LITERAL_CSTRING("startup_time"),
                                  mStartupTime);
    RETURN_IF_FAILED(rv);

    addPage->Execute();
  }
}


NS_IMETHODIMP
Seer::Learn(nsIURI *targetURI, nsIURI *sourceURI, SeerLearnReason reason,
            nsILoadContext *loadContext)
{
  MOZ_ASSERT(NS_IsMainThread(),
             "Seer interface methods must be called on the main thread");

  if (!mInitialized) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!mEnabled) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (loadContext && loadContext->UsePrivateBrowsing()) {
    
    return NS_OK;
  }

  if (!IsNullOrHttp(targetURI) || !IsNullOrHttp(sourceURI)) {
    return NS_ERROR_INVALID_ARG;
  }

  switch (reason) {
  case nsINetworkSeer::LEARN_LOAD_TOPLEVEL:
  case nsINetworkSeer::LEARN_STARTUP:
    if (!targetURI || sourceURI) {
      return NS_ERROR_INVALID_ARG;
    }
    break;
  case nsINetworkSeer::LEARN_LOAD_REDIRECT:
  case nsINetworkSeer::LEARN_LOAD_SUBRESOURCE:
    if (!targetURI || !sourceURI) {
      return NS_ERROR_INVALID_ARG;
    }
    break;
  default:
    return NS_ERROR_INVALID_ARG;
  }

  ++mAccumulators->mLearnAttempts;
  nsresult rv = ReserveSpaceInQueue();
  if (NS_FAILED(rv)) {
    ++mAccumulators->mLearnFullQueue;
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<SeerLearnEvent> event = new SeerLearnEvent(targetURI, sourceURI,
                                                      reason);
  return mIOThread->Dispatch(event, NS_DISPATCH_NORMAL);
}



class SeerResetEvent : public nsRunnable
{
public:
  SeerResetEvent()
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!NS_IsMainThread(), "Running reset on main thread");

    gSeer->ResetInternal();

    return NS_OK;
  }
};


void
Seer::ResetInternal()
{
  MOZ_ASSERT(!NS_IsMainThread(), "Resetting db on main thread");

  nsresult rv = EnsureInitStorage();
  RETURN_IF_FAILED(rv);

  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM moz_redirects"));
  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM moz_startup_pages"));
  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM moz_startups"));

  
  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM moz_pages"));
  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM moz_hosts"));
}


NS_IMETHODIMP
Seer::Reset()
{
  MOZ_ASSERT(NS_IsMainThread(),
             "Seer interface methods must be called on the main thread");

  if (!mInitialized) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<SeerResetEvent> event = new SeerResetEvent();
  return mIOThread->Dispatch(event, NS_DISPATCH_NORMAL);
}



static nsresult
EnsureGlobalSeer(nsINetworkSeer **aSeer)
{
  nsresult rv;
  nsCOMPtr<nsINetworkSeer> seer = do_GetService("@mozilla.org/network/seer;1",
                                                &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aSeer = seer);
  return NS_OK;
}

nsresult
SeerPredict(nsIURI *targetURI, nsIURI *sourceURI, SeerPredictReason reason,
            nsILoadContext *loadContext, nsINetworkSeerVerifier *verifier)
{
  nsCOMPtr<nsINetworkSeer> seer;
  nsresult rv = EnsureGlobalSeer(getter_AddRefs(seer));
  NS_ENSURE_SUCCESS(rv, rv);

  return seer->Predict(targetURI, sourceURI, reason, loadContext, verifier);
}

nsresult
SeerLearn(nsIURI *targetURI, nsIURI *sourceURI, SeerLearnReason reason,
          nsILoadContext *loadContext)
{
  nsCOMPtr<nsINetworkSeer> seer;
  nsresult rv = EnsureGlobalSeer(getter_AddRefs(seer));
  NS_ENSURE_SUCCESS(rv, rv);

  return seer->Learn(targetURI, sourceURI, reason, loadContext);
}

nsresult
SeerLearn(nsIURI *targetURI, nsIURI *sourceURI, SeerLearnReason reason,
          nsILoadGroup *loadGroup)
{
  nsCOMPtr<nsINetworkSeer> seer;
  nsresult rv = EnsureGlobalSeer(getter_AddRefs(seer));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILoadContext> loadContext;

  if (loadGroup) {
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    loadGroup->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (callbacks) {
      loadContext = do_GetInterface(callbacks);
    }
  }

  return seer->Learn(targetURI, sourceURI, reason, loadContext);
}

nsresult
SeerLearn(nsIURI *targetURI, nsIURI *sourceURI, SeerLearnReason reason,
          nsIDocument *document)
{
  nsCOMPtr<nsINetworkSeer> seer;
  nsresult rv = EnsureGlobalSeer(getter_AddRefs(seer));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILoadContext> loadContext;

  if (document) {
    loadContext = document->GetLoadContext();
  }

  return seer->Learn(targetURI, sourceURI, reason, loadContext);
}

nsresult
SeerLearnRedirect(nsIURI *targetURI, nsIChannel *channel,
                  nsILoadContext *loadContext)
{
  nsCOMPtr<nsINetworkSeer> seer;
  nsresult rv = EnsureGlobalSeer(getter_AddRefs(seer));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> sourceURI;
  rv = channel->GetOriginalURI(getter_AddRefs(sourceURI));
  NS_ENSURE_SUCCESS(rv, rv);

  bool sameUri;
  rv = targetURI->Equals(sourceURI, &sameUri);
  NS_ENSURE_SUCCESS(rv, rv);

  if (sameUri) {
    return NS_OK;
  }

  return seer->Learn(targetURI, sourceURI,
                     nsINetworkSeer::LEARN_LOAD_REDIRECT, loadContext);
}

} 
} 
