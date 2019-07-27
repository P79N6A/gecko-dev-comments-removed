




#include "mozilla/DebugOnly.h"

#include "mozIStorageService.h"
#include "nsIAlertsService.h"
#include "nsIArray.h"
#include "nsIClassInfoImpl.h"
#include "nsIDOMWindow.h"
#include "nsIDownloadHistory.h"
#include "nsIDownloadManagerUI.h"
#include "nsIMIMEService.h"
#include "nsIParentalControlsService.h"
#include "nsIPrefService.h"
#include "nsIPromptService.h"
#include "nsIResumableChannel.h"
#include "nsIWebBrowserPersist.h"
#include "nsIWindowMediator.h"
#include "nsILocalFileWin.h"
#include "nsILoadContext.h"
#include "nsIXULAppInfo.h"
#include "nsContentUtils.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsArrayEnumerator.h"
#include "nsCExternalHandlerService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDownloadManager.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"

#include "mozStorageCID.h"
#include "nsDocShellCID.h"
#include "nsEmbedCID.h"
#include "nsToolkitCompsCID.h"

#include "mozilla/net/ReferrerPolicy.h"

#include "SQLFunctions.h"

#include "mozilla/Preferences.h"

#ifdef XP_WIN
#include <shlobj.h>
#include "nsWindowsHelpers.h"
#ifdef DOWNLOAD_SCANNER
#include "nsDownloadScanner.h"
#endif
#endif

#ifdef XP_MACOSX
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#endif

#ifdef MOZ_WIDGET_GTK
#include <gtk/gtk.h>
#endif

using namespace mozilla;
using mozilla::downloads::GenerateGUID;

#define DOWNLOAD_MANAGER_BUNDLE "chrome://mozapps/locale/downloads/downloads.properties"
#define DOWNLOAD_MANAGER_ALERT_ICON "chrome://mozapps/skin/downloads/downloadIcon.png"
#define PREF_BD_USEJSTRANSFER "browser.download.useJSTransfer"
#define PREF_BDM_SHOWALERTONCOMPLETE "browser.download.manager.showAlertOnComplete"
#define PREF_BDM_SHOWALERTINTERVAL "browser.download.manager.showAlertInterval"
#define PREF_BDM_RETENTION "browser.download.manager.retention"
#define PREF_BDM_QUITBEHAVIOR "browser.download.manager.quitBehavior"
#define PREF_BDM_ADDTORECENTDOCS "browser.download.manager.addToRecentDocs"
#define PREF_BDM_SCANWHENDONE "browser.download.manager.scanWhenDone"
#define PREF_BDM_RESUMEONWAKEDELAY "browser.download.manager.resumeOnWakeDelay"
#define PREF_BH_DELETETEMPFILEONEXIT "browser.helperApps.deleteTempFileOnExit"

static const int64_t gUpdateInterval = 400 * PR_USEC_PER_MSEC;

#define DM_SCHEMA_VERSION      9
#define DM_DB_NAME             NS_LITERAL_STRING("downloads.sqlite")
#define DM_DB_CORRUPT_FILENAME NS_LITERAL_STRING("downloads.sqlite.corrupt")

#define NS_SYSTEMINFO_CONTRACTID "@mozilla.org/system-info;1"




NS_IMPL_ISUPPORTS(
  nsDownloadManager
, nsIDownloadManager
, nsINavHistoryObserver
, nsIObserver
, nsISupportsWeakReference
)

nsDownloadManager *nsDownloadManager::gDownloadManagerService = nullptr;

nsDownloadManager *
nsDownloadManager::GetSingleton()
{
  if (gDownloadManagerService) {
    NS_ADDREF(gDownloadManagerService);
    return gDownloadManagerService;
  }

  gDownloadManagerService = new nsDownloadManager();
  if (gDownloadManagerService) {
#if defined(MOZ_WIDGET_GTK)
    g_type_init();
#endif
    NS_ADDREF(gDownloadManagerService);
    if (NS_FAILED(gDownloadManagerService->Init()))
      NS_RELEASE(gDownloadManagerService);
  }

  return gDownloadManagerService;
}

nsDownloadManager::~nsDownloadManager()
{
#ifdef DOWNLOAD_SCANNER
  if (mScanner) {
    delete mScanner;
    mScanner = nullptr;
  }
#endif
  gDownloadManagerService = nullptr;
}

nsresult
nsDownloadManager::ResumeRetry(nsDownload *aDl)
{
  
  nsRefPtr<nsDownload> dl = aDl;

  
  nsresult rv = dl->Resume();

  
  if (NS_FAILED(rv)) {
    
    rv = dl->Cancel();

    
    if (NS_SUCCEEDED(rv))
      rv = dl->Retry();
  }

  return rv;
}

nsresult
nsDownloadManager::PauseAllDownloads(bool aSetResume)
{
  nsresult rv = PauseAllDownloads(mCurrentDownloads, aSetResume);
  nsresult rv2 = PauseAllDownloads(mCurrentPrivateDownloads, aSetResume);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(rv2, rv2);
  return NS_OK;
}

nsresult
nsDownloadManager::PauseAllDownloads(nsCOMArray<nsDownload>& aDownloads, bool aSetResume)
{
  nsresult retVal = NS_OK;
  for (int32_t i = aDownloads.Count() - 1; i >= 0; --i) {
    nsRefPtr<nsDownload> dl = aDownloads[i];

    
    if (!dl->IsPaused()) {
      
      dl->mAutoResume = aSetResume ? nsDownload::AUTO_RESUME :
                                     nsDownload::DONT_RESUME;

      
      nsresult rv = dl->Pause();
      if (NS_FAILED(rv))
        retVal = rv;
    }
  }

  return retVal;
}

nsresult
nsDownloadManager::ResumeAllDownloads(bool aResumeAll)
{
  nsresult rv = ResumeAllDownloads(mCurrentDownloads, aResumeAll);
  nsresult rv2 = ResumeAllDownloads(mCurrentPrivateDownloads, aResumeAll);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(rv2, rv2);
  return NS_OK;
}

nsresult
nsDownloadManager::ResumeAllDownloads(nsCOMArray<nsDownload>& aDownloads, bool aResumeAll)
{
  nsresult retVal = NS_OK;
  for (int32_t i = aDownloads.Count() - 1; i >= 0; --i) {
    nsRefPtr<nsDownload> dl = aDownloads[i];

    
    
    if (aResumeAll || dl->ShouldAutoResume()) {
      
      
      
      
      dl->mAutoResume = nsDownload::DONT_RESUME;

      
      nsresult rv = ResumeRetry(dl);
      if (NS_FAILED(rv))
        retVal = rv;
    }
  }

  return retVal;
}

nsresult
nsDownloadManager::RemoveAllDownloads()
{
  nsresult rv = RemoveAllDownloads(mCurrentDownloads);
  nsresult rv2 = RemoveAllDownloads(mCurrentPrivateDownloads);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(rv2, rv2);
  return NS_OK;
}

nsresult
nsDownloadManager::RemoveAllDownloads(nsCOMArray<nsDownload>& aDownloads)
{
  nsresult rv = NS_OK;
  for (int32_t i = aDownloads.Count() - 1; i >= 0; --i) {
    nsRefPtr<nsDownload> dl = aDownloads[0];

    nsresult result = NS_OK;
    if (!dl->mPrivate && dl->IsPaused() && GetQuitBehavior() != QUIT_AND_CANCEL)
      aDownloads.RemoveObject(dl);
    else
      result = dl->Cancel();

    
    if (NS_FAILED(result))
      rv = result;
  }

  return rv;
}

nsresult
nsDownloadManager::RemoveDownloadsForURI(mozIStorageStatement* aStatement, nsIURI *aURI)
{
  mozStorageStatementScoper scope(aStatement);

  nsAutoCString source;
  nsresult rv = aURI->GetSpec(source);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStatement->BindUTF8StringByName(
    NS_LITERAL_CSTRING("source"), source);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = false;
  nsAutoTArray<nsCString, 4> downloads;
  
  while (NS_SUCCEEDED(aStatement->ExecuteStep(&hasMore)) &&
         hasMore) {
    nsAutoCString downloadGuid;
    rv = aStatement->GetUTF8String(0, downloadGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    downloads.AppendElement(downloadGuid);
  }

  
  for (int32_t i = downloads.Length(); --i >= 0; )
    (void)RemoveDownload(downloads[i]);

  return NS_OK;
}

void 
nsDownloadManager::ResumeOnWakeCallback(nsITimer *aTimer, void *aClosure)
{
  
  nsDownloadManager *dlMgr = static_cast<nsDownloadManager *>(aClosure);
  (void)dlMgr->ResumeAllDownloads(false);
}

already_AddRefed<mozIStorageConnection>
nsDownloadManager::GetFileDBConnection(nsIFile *dbFile) const
{
  NS_ASSERTION(dbFile, "GetFileDBConnection called with an invalid nsIFile");

  nsCOMPtr<mozIStorageService> storage =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(storage, nullptr);

  nsCOMPtr<mozIStorageConnection> conn;
  nsresult rv = storage->OpenDatabase(dbFile, getter_AddRefs(conn));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    
    rv = dbFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, nullptr);
    rv = storage->OpenDatabase(dbFile, getter_AddRefs(conn));
  }
  NS_ENSURE_SUCCESS(rv, nullptr);

  return conn.forget();
}

already_AddRefed<mozIStorageConnection>
nsDownloadManager::GetPrivateDBConnection() const
{
  nsCOMPtr<mozIStorageService> storage =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(storage, nullptr);

  nsCOMPtr<mozIStorageConnection> conn;
  nsresult rv = storage->OpenSpecialDatabase("memory", getter_AddRefs(conn));
  NS_ENSURE_SUCCESS(rv, nullptr);

  return conn.forget();
}

void
nsDownloadManager::CloseAllDBs()
{
  CloseDB(mDBConn, mUpdateDownloadStatement, mGetIdsForURIStatement);
  CloseDB(mPrivateDBConn, mUpdatePrivateDownloadStatement, mGetPrivateIdsForURIStatement);
}

void
nsDownloadManager::CloseDB(mozIStorageConnection* aDBConn,
                           mozIStorageStatement* aUpdateStmt,
                           mozIStorageStatement* aGetIdsStmt)
{
  DebugOnly<nsresult> rv = aGetIdsStmt->Finalize();
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  rv = aUpdateStmt->Finalize();
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  rv = aDBConn->AsyncClose(nullptr);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
}

static nsresult
InitSQLFunctions(mozIStorageConnection* aDBConn)
{
  nsresult rv = mozilla::downloads::GenerateGUIDFunction::create(aDBConn);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsDownloadManager::InitPrivateDB()
{
  bool ready = false;
  if (mPrivateDBConn && NS_SUCCEEDED(mPrivateDBConn->GetConnectionReady(&ready)) && ready)
    CloseDB(mPrivateDBConn, mUpdatePrivateDownloadStatement, mGetPrivateIdsForURIStatement);
  mPrivateDBConn = GetPrivateDBConnection();
  if (!mPrivateDBConn)
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = InitSQLFunctions(mPrivateDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateTable(mPrivateDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitStatements(mPrivateDBConn, getter_AddRefs(mUpdatePrivateDownloadStatement),
                      getter_AddRefs(mGetPrivateIdsForURIStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsDownloadManager::InitFileDB()
{
  nsresult rv;

  nsCOMPtr<nsIFile> dbFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(dbFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = dbFile->Append(DM_DB_NAME);
  NS_ENSURE_SUCCESS(rv, rv);

  bool ready = false;
  if (mDBConn && NS_SUCCEEDED(mDBConn->GetConnectionReady(&ready)) && ready)
    CloseDB(mDBConn, mUpdateDownloadStatement, mGetIdsForURIStatement);
  mDBConn = GetFileDBConnection(dbFile);
  NS_ENSURE_TRUE(mDBConn, NS_ERROR_NOT_AVAILABLE);

  rv = InitSQLFunctions(mDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  bool tableExists;
  rv = mDBConn->TableExists(NS_LITERAL_CSTRING("moz_downloads"), &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!tableExists) {
    rv = CreateTable(mDBConn);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    return NS_OK;
  }

  
  int32_t schemaVersion;
  rv = mDBConn->GetSchemaVersion(&schemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  

  switch (schemaVersion) {
  
  
  
  
  case 1: 
    {
      
      mozStorageTransaction safeTransaction(mDBConn, true);

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE TEMPORARY TABLE moz_downloads_backup ("
          "id INTEGER PRIMARY KEY, "
          "name TEXT, "
          "source TEXT, "
          "target TEXT, "
          "startTime INTEGER, "
          "endTime INTEGER, "
          "state INTEGER"
        ")"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "INSERT INTO moz_downloads_backup "
        "SELECT id, name, source, target, startTime, endTime, state "
        "FROM moz_downloads"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP TABLE moz_downloads"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE TABLE moz_downloads ("
          "id INTEGER PRIMARY KEY, "
          "name TEXT, "
          "source TEXT, "
          "target TEXT, "
          "startTime INTEGER, "
          "endTime INTEGER, "
          "state INTEGER"
        ")"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "INSERT INTO moz_downloads "
        "SELECT id, name, source, target, startTime, endTime, state "
        "FROM moz_downloads_backup"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP TABLE moz_downloads_backup"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      schemaVersion = 2;
      rv = mDBConn->SetSchemaVersion(schemaVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  case 2: 
    {
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN referrer TEXT"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      schemaVersion = 3;
      rv = mDBConn->SetSchemaVersion(schemaVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  case 3: 
    {
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN entityID TEXT"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      schemaVersion = 4;
      rv = mDBConn->SetSchemaVersion(schemaVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  case 4: 
    {
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN tempPath TEXT"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      schemaVersion = 5;
      rv = mDBConn->SetSchemaVersion(schemaVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  case 5: 
    {
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN currBytes INTEGER NOT NULL DEFAULT 0"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN maxBytes INTEGER NOT NULL DEFAULT -1"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      schemaVersion = 6;
      rv = mDBConn->SetSchemaVersion(schemaVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  case 6: 
    {
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN mimeType TEXT"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN preferredApplication TEXT"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN preferredAction INTEGER NOT NULL DEFAULT 0"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      schemaVersion = 7;
      rv = mDBConn->SetSchemaVersion(schemaVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  case 7: 
    {
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_downloads "
        "ADD COLUMN autoResume INTEGER NOT NULL DEFAULT 0"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      schemaVersion = 8;
      rv = mDBConn->SetSchemaVersion(schemaVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

    
    
    
    

  case 8: 
    {
      bool exists;
      rv = mDBConn->IndexExists(NS_LITERAL_CSTRING("moz_downloads_guid_uniqueindex"),
                                &exists);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!exists) {
        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "ALTER TABLE moz_downloads ADD COLUMN guid TEXT"));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "CREATE UNIQUE INDEX moz_downloads_guid_uniqueindex ON moz_downloads (guid)"));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "UPDATE moz_downloads SET guid = GENERATE_GUID() WHERE guid ISNULL"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      schemaVersion = 9;
      rv = mDBConn->SetSchemaVersion(schemaVersion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  
#ifndef DEBUG
  case DM_SCHEMA_VERSION:
#endif
    break;

  case 0:
    {
      NS_WARNING("Could not get download database's schema version!");

      
      
      
      
      rv = mDBConn->SetSchemaVersion(DM_SCHEMA_VERSION);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  
  
  
  
  
  
  default:
    {
      nsCOMPtr<mozIStorageStatement> stmt;
      rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT id, name, source, target, tempPath, startTime, endTime, state, "
               "referrer, entityID, currBytes, maxBytes, mimeType, "
               "preferredApplication, preferredAction, autoResume, guid "
        "FROM moz_downloads"), getter_AddRefs(stmt));
      if (NS_SUCCEEDED(rv)) {
        
        
        
        mDBConn->SetSchemaVersion(DM_SCHEMA_VERSION);
        break;
      }

      
      
      nsCOMPtr<mozIStorageService> storage =
        do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
      NS_ENSURE_TRUE(storage, NS_ERROR_NOT_AVAILABLE);
      nsCOMPtr<nsIFile> backup;
      rv = storage->BackupDatabaseFile(dbFile, DM_DB_CORRUPT_FILENAME, nullptr,
                                       getter_AddRefs(backup));
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP TABLE moz_downloads"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = CreateTable(mDBConn);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;
  }

  return NS_OK;
}

nsresult
nsDownloadManager::CreateTable(mozIStorageConnection* aDBConn)
{
  nsresult rv = aDBConn->SetSchemaVersion(DM_SCHEMA_VERSION);
  if (NS_FAILED(rv)) return rv;

  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE moz_downloads ("
      "id INTEGER PRIMARY KEY, "
      "name TEXT, "
      "source TEXT, "
      "target TEXT, "
      "tempPath TEXT, "
      "startTime INTEGER, "
      "endTime INTEGER, "
      "state INTEGER, "
      "referrer TEXT, "
      "entityID TEXT, "
      "currBytes INTEGER NOT NULL DEFAULT 0, "
      "maxBytes INTEGER NOT NULL DEFAULT -1, "
      "mimeType TEXT, "
      "preferredApplication TEXT, "
      "preferredAction INTEGER NOT NULL DEFAULT 0, "
      "autoResume INTEGER NOT NULL DEFAULT 0, "
      "guid TEXT"
    ")"));
  if (NS_FAILED(rv)) return rv;

  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE UNIQUE INDEX moz_downloads_guid_uniqueindex "
      "ON moz_downloads(guid)"));
  return rv;
}

nsresult
nsDownloadManager::RestoreDatabaseState()
{
  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_downloads "
    "SET state = :state "
    "WHERE state = :state_cond"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("state"), nsIDownloadManager::DOWNLOAD_FINISHED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("state_cond"), nsIDownloadManager::DOWNLOAD_SCANNING);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_downloads "
    "SET autoResume = :autoResume "
    "WHERE state = :notStarted "
      "OR state = :queued "
      "OR state = :downloading"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("autoResume"), nsDownload::AUTO_RESUME);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("notStarted"), nsIDownloadManager::DOWNLOAD_NOTSTARTED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("queued"), nsIDownloadManager::DOWNLOAD_QUEUED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("downloading"), nsIDownloadManager::DOWNLOAD_DOWNLOADING);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_downloads "
    "SET autoResume = :autoResume "
    "WHERE state = :state "
      "AND autoResume = :autoResume_cond"),
    getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("autoResume"), nsDownload::DONT_RESUME);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("state"), nsIDownloadManager::DOWNLOAD_FINISHED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("autoResume_cond"), nsDownload::AUTO_RESUME);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDownloadManager::RestoreActiveDownloads()
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id "
    "FROM moz_downloads "
    "WHERE (state = :state AND LENGTH(entityID) > 0) "
      "OR autoResume != :autoResume"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("state"), nsIDownloadManager::DOWNLOAD_PAUSED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("autoResume"), nsDownload::DONT_RESUME);
  NS_ENSURE_SUCCESS(rv, rv);

  nsresult retVal = NS_OK;
  bool hasResults;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResults)) && hasResults) {
    nsRefPtr<nsDownload> dl;
    
    
    
    if (NS_FAILED(GetDownloadFromDB(stmt->AsInt32(0), getter_AddRefs(dl))) ||
        NS_FAILED(AddToCurrentDownloads(dl)))
      retVal = NS_ERROR_FAILURE;
  }

  
  rv = ResumeAllDownloads(false);
  NS_ENSURE_SUCCESS(rv, rv);

  return retVal;
}

int64_t
nsDownloadManager::AddDownloadToDB(const nsAString &aName,
                                   const nsACString &aSource,
                                   const nsACString &aTarget,
                                   const nsAString &aTempPath,
                                   int64_t aStartTime,
                                   int64_t aEndTime,
                                   const nsACString &aMimeType,
                                   const nsACString &aPreferredApp,
                                   nsHandlerInfoAction aPreferredAction,
                                   bool aPrivate,
                                   nsACString& aNewGUID)
{
  mozIStorageConnection* dbConn = aPrivate ? mPrivateDBConn : mDBConn;
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = dbConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_downloads "
    "(name, source, target, tempPath, startTime, endTime, state, "
     "mimeType, preferredApplication, preferredAction, guid) VALUES "
    "(:name, :source, :target, :tempPath, :startTime, :endTime, :state, "
     ":mimeType, :preferredApplication, :preferredAction, :guid)"),
    getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("name"), aName);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("source"), aSource);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("target"), aTarget);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("tempPath"), aTempPath);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("startTime"), aStartTime);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("endTime"), aEndTime);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("state"), nsIDownloadManager::DOWNLOAD_NOTSTARTED);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("mimeType"), aMimeType);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("preferredApplication"), aPreferredApp);
  NS_ENSURE_SUCCESS(rv, 0);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("preferredAction"), aPreferredAction);
  NS_ENSURE_SUCCESS(rv, 0);

  nsAutoCString guid;
  rv = GenerateGUID(guid);
  NS_ENSURE_SUCCESS(rv, 0);
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), guid);
  NS_ENSURE_SUCCESS(rv, 0);

  bool hasMore;
  rv = stmt->ExecuteStep(&hasMore); 
  NS_ENSURE_SUCCESS(rv, 0);

  int64_t id = 0;
  rv = dbConn->GetLastInsertRowID(&id);
  NS_ENSURE_SUCCESS(rv, 0);

  aNewGUID = guid;

  
  return id;
}

nsresult
nsDownloadManager::InitDB()
{
  nsresult rv = InitPrivateDB();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitFileDB();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitStatements(mDBConn, getter_AddRefs(mUpdateDownloadStatement),
                      getter_AddRefs(mGetIdsForURIStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsDownloadManager::InitStatements(mozIStorageConnection* aDBConn,
                                  mozIStorageStatement** aUpdateStatement,
                                  mozIStorageStatement** aGetIdsStatement)
{
  nsresult rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_downloads "
    "SET tempPath = :tempPath, startTime = :startTime, endTime = :endTime, "
      "state = :state, referrer = :referrer, entityID = :entityID, "
      "currBytes = :currBytes, maxBytes = :maxBytes, autoResume = :autoResume "
    "WHERE id = :id"), aUpdateStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT guid "
    "FROM moz_downloads "
    "WHERE source = :source"), aGetIdsStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDownloadManager::Init()
{
  nsresult rv;

  nsCOMPtr<nsIStringBundleService> bundleService =
    mozilla::services::GetStringBundleService();
  if (!bundleService)
    return NS_ERROR_FAILURE;

  rv = bundleService->CreateBundle(DOWNLOAD_MANAGER_BUNDLE,
                                   getter_AddRefs(mBundle));
  NS_ENSURE_SUCCESS(rv, rv);

#if !defined(MOZ_JSDOWNLOADS)
  
  
  mUseJSTransfer = Preferences::GetBool(PREF_BD_USEJSTRANSFER, false);
#elif defined(XP_WIN)
    
    
    
    mUseJSTransfer = !IsRunningInWindowsMetro();
#else
    mUseJSTransfer = true;
#endif

  if (mUseJSTransfer)
    return NS_OK;

  
  {
    nsCOMPtr<nsIFile> oldDownloadsFile;
    bool fileExists;
    if (NS_SUCCEEDED(NS_GetSpecialDirectory(NS_APP_DOWNLOADS_50_FILE,
          getter_AddRefs(oldDownloadsFile))) &&
        NS_SUCCEEDED(oldDownloadsFile->Exists(&fileExists)) &&
        fileExists) {
      (void)oldDownloadsFile->Remove(false);
    }
  }

  mObserverService = mozilla::services::GetObserverService();
  if (!mObserverService)
    return NS_ERROR_FAILURE;

  rv = InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DOWNLOAD_SCANNER
  mScanner = new nsDownloadScanner();
  if (!mScanner)
    return NS_ERROR_OUT_OF_MEMORY;
  rv = mScanner->Init();
  if (NS_FAILED(rv)) {
    delete mScanner;
    mScanner = nullptr;
  }
#endif

  
  
  rv = RestoreDatabaseState();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RestoreActiveDownloads();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to restore all active downloads");

  nsCOMPtr<nsINavHistoryService> history =
    do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);

  (void)mObserverService->NotifyObservers(
                                static_cast<nsIDownloadManager *>(this),
                                "download-manager-initialized",
                                nullptr);

  
  
  
  
  
  (void)mObserverService->AddObserver(this, "quit-application", true);
  (void)mObserverService->AddObserver(this, "quit-application-requested", true);
  (void)mObserverService->AddObserver(this, "offline-requested", true);
  (void)mObserverService->AddObserver(this, "sleep_notification", true);
  (void)mObserverService->AddObserver(this, "wake_notification", true);
  (void)mObserverService->AddObserver(this, "suspend_process_notification", true);
  (void)mObserverService->AddObserver(this, "resume_process_notification", true);
  (void)mObserverService->AddObserver(this, "profile-before-change", true);
  (void)mObserverService->AddObserver(this, NS_IOSERVICE_GOING_OFFLINE_TOPIC, true);
  (void)mObserverService->AddObserver(this, NS_IOSERVICE_OFFLINE_STATUS_TOPIC, true);
  (void)mObserverService->AddObserver(this, "last-pb-context-exited", true);
  (void)mObserverService->AddObserver(this, "last-pb-context-exiting", true);

  if (history)
    (void)history->AddObserver(this, true);

  return NS_OK;
}

int32_t
nsDownloadManager::GetRetentionBehavior()
{
  
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, 0);

  int32_t val;
  rv = pref->GetIntPref(PREF_BDM_RETENTION, &val);
  NS_ENSURE_SUCCESS(rv, 0);

  
  
  
  
  
  nsCOMPtr<nsISupportsPRInt32> retentionBehavior =
    do_CreateInstance(NS_SUPPORTS_PRINT32_CONTRACTID);
  retentionBehavior->SetData(val);
  (void)mObserverService->NotifyObservers(retentionBehavior,
                                          "download-manager-change-retention",
                                          nullptr);
  retentionBehavior->GetData(&val);

  return val;
}

enum nsDownloadManager::QuitBehavior
nsDownloadManager::GetQuitBehavior()
{
  
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, QUIT_AND_RESUME);

  int32_t val;
  rv = pref->GetIntPref(PREF_BDM_QUITBEHAVIOR, &val);
  NS_ENSURE_SUCCESS(rv, QUIT_AND_RESUME);

  switch (val) {
    case 1:
      return QUIT_AND_PAUSE;
    case 2:
      return QUIT_AND_CANCEL;
    default:
      return QUIT_AND_RESUME;
  }
}





nsresult
nsDownloadManager::GetDownloadFromDB(const nsACString& aGUID, nsDownload **retVal)
{
  MOZ_ASSERT(!FindDownload(aGUID),
             "If it is a current download, you should not call this method!");

  NS_NAMED_LITERAL_CSTRING(query,
    "SELECT id, state, startTime, source, target, tempPath, name, referrer, "
           "entityID, currBytes, maxBytes, mimeType, preferredAction, "
           "preferredApplication, autoResume, guid "
    "FROM moz_downloads "
    "WHERE guid = :guid");
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(query, getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetDownloadFromDB(mDBConn, stmt, retVal);

  
  
  
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    rv = mPrivateDBConn->CreateStatement(query, getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), aGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetDownloadFromDB(mPrivateDBConn, stmt, retVal);

    
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      *retVal = nullptr;
    }
  }
  return rv;
}

nsresult
nsDownloadManager::GetDownloadFromDB(uint32_t aID, nsDownload **retVal)
{
  NS_WARNING("Using integer IDs without compat mode enabled");

  MOZ_ASSERT(!FindDownload(aID),
             "If it is a current download, you should not call this method!");

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, state, startTime, source, target, tempPath, name, referrer, "
           "entityID, currBytes, maxBytes, mimeType, preferredAction, "
           "preferredApplication, autoResume, guid "
    "FROM moz_downloads "
    "WHERE id = :id"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"), aID);
  NS_ENSURE_SUCCESS(rv, rv);

  return GetDownloadFromDB(mDBConn, stmt, retVal);
}

nsresult
nsDownloadManager::GetDownloadFromDB(mozIStorageConnection* aDBConn,
                                     mozIStorageStatement* stmt,
                                     nsDownload **retVal)
{
  bool hasResults = false;
  nsresult rv = stmt->ExecuteStep(&hasResults);
  if (NS_FAILED(rv) || !hasResults)
    return NS_ERROR_NOT_AVAILABLE;

  
  nsRefPtr<nsDownload> dl = new nsDownload();
  if (!dl)
    return NS_ERROR_OUT_OF_MEMORY;
  dl->mPrivate = aDBConn == mPrivateDBConn;

  dl->mDownloadManager = this;

  int32_t i = 0;
  
  dl->mCancelable = nullptr;
  dl->mID = stmt->AsInt64(i++);
  dl->mDownloadState = stmt->AsInt32(i++);
  dl->mStartTime = stmt->AsInt64(i++);

  nsCString source;
  stmt->GetUTF8String(i++, source);
  rv = NS_NewURI(getter_AddRefs(dl->mSource), source);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString target;
  stmt->GetUTF8String(i++, target);
  rv = NS_NewURI(getter_AddRefs(dl->mTarget), target);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString tempPath;
  stmt->GetString(i++, tempPath);
  if (!tempPath.IsEmpty()) {
    rv = NS_NewLocalFile(tempPath, true, getter_AddRefs(dl->mTempFile));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  stmt->GetString(i++, dl->mDisplayName);

  nsCString referrer;
  rv = stmt->GetUTF8String(i++, referrer);
  if (NS_SUCCEEDED(rv) && !referrer.IsEmpty()) {
    rv = NS_NewURI(getter_AddRefs(dl->mReferrer), referrer);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = stmt->GetUTF8String(i++, dl->mEntityID);
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t currBytes = stmt->AsInt64(i++);
  int64_t maxBytes = stmt->AsInt64(i++);
  dl->SetProgressBytes(currBytes, maxBytes);

  
  nsAutoCString mimeType;
  rv = stmt->GetUTF8String(i++, mimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mimeType.IsEmpty()) {
    nsCOMPtr<nsIMIMEService> mimeService =
      do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mimeService->GetFromTypeAndExtension(mimeType, EmptyCString(),
                                              getter_AddRefs(dl->mMIMEInfo));
    NS_ENSURE_SUCCESS(rv, rv);

    nsHandlerInfoAction action = stmt->AsInt32(i++);
    rv = dl->mMIMEInfo->SetPreferredAction(action);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString persistentDescriptor;
    rv = stmt->GetUTF8String(i++, persistentDescriptor);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!persistentDescriptor.IsEmpty()) {
      nsCOMPtr<nsILocalHandlerApp> handler =
        do_CreateInstance(NS_LOCALHANDLERAPP_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIFile> localExecutable;
      rv = NS_NewNativeLocalFile(EmptyCString(), false,
                                 getter_AddRefs(localExecutable));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = localExecutable->SetPersistentDescriptor(persistentDescriptor);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = handler->SetExecutable(localExecutable);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = dl->mMIMEInfo->SetPreferredApplicationHandler(handler);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    
    i += 2;
  }

  dl->mAutoResume =
    static_cast<enum nsDownload::AutoResume>(stmt->AsInt32(i++));

  rv = stmt->GetUTF8String(i++, dl->mGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (dl->mGUID.IsEmpty()) {
    rv = GenerateGUID(dl->mGUID);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<mozIStorageStatement> stmt;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
                                    "UPDATE moz_downloads SET guid = :guid "
                                    "WHERE id = :id"),
                                  getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), dl->mGUID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"), dl->mID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  dl.forget(retVal);
  return NS_OK;
}

nsresult
nsDownloadManager::AddToCurrentDownloads(nsDownload *aDl)
{
  nsCOMArray<nsDownload>& currentDownloads =
    aDl->mPrivate ? mCurrentPrivateDownloads : mCurrentDownloads;
  if (!currentDownloads.AppendObject(aDl))
    return NS_ERROR_OUT_OF_MEMORY;

  aDl->mDownloadManager = this;
  return NS_OK;
}

void
nsDownloadManager::SendEvent(nsDownload *aDownload, const char *aTopic)
{
  (void)mObserverService->NotifyObservers(aDownload, aTopic, nullptr);
}




NS_IMETHODIMP
nsDownloadManager::GetActivePrivateDownloadCount(int32_t* aResult)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  *aResult = mCurrentPrivateDownloads.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetActiveDownloadCount(int32_t *aResult)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  *aResult = mCurrentDownloads.Count();

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetActiveDownloads(nsISimpleEnumerator **aResult)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  return NS_NewArrayEnumerator(aResult, mCurrentDownloads);
}

NS_IMETHODIMP
nsDownloadManager::GetActivePrivateDownloads(nsISimpleEnumerator **aResult)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  return NS_NewArrayEnumerator(aResult, mCurrentPrivateDownloads);
}





NS_IMETHODIMP
nsDownloadManager::GetDefaultDownloadsDirectory(nsIFile **aResult)
{
  nsCOMPtr<nsIFile> downloadDir;

  nsresult rv;
  nsCOMPtr<nsIProperties> dirService =
     do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  

  nsXPIDLString folderName;
  mBundle->GetStringFromName(MOZ_UTF16("downloadsFolder"),
                             getter_Copies(folderName));

#if defined (XP_MACOSX)
  rv = dirService->Get(NS_OSX_DEFAULT_DOWNLOAD_DIR,
                       NS_GET_IID(nsIFile),
                       getter_AddRefs(downloadDir));
  NS_ENSURE_SUCCESS(rv, rv);
#elif defined(XP_WIN)
  rv = dirService->Get(NS_WIN_DEFAULT_DOWNLOAD_DIR,
                       NS_GET_IID(nsIFile),
                       getter_AddRefs(downloadDir));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPropertyBag2> infoService =
     do_GetService(NS_SYSTEMINFO_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t version;
  NS_NAMED_LITERAL_STRING(osVersion, "version");
  rv = infoService->GetPropertyAsInt32(osVersion, &version);
  NS_ENSURE_SUCCESS(rv, rv);
  if (version < 6) { 
    
    rv = dirService->Get(NS_WIN_PERSONAL_DIR,
                         NS_GET_IID(nsIFile),
                         getter_AddRefs(downloadDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = downloadDir->Append(folderName);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    bool exists;
    rv = downloadDir->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!exists) {
      rv = downloadDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
#elif defined(XP_UNIX)
#if defined(MOZ_WIDGET_ANDROID)
    
    
    char* downloadDirPath = getenv("DOWNLOADS_DIRECTORY");
    if (downloadDirPath) {
      rv = NS_NewNativeLocalFile(nsDependentCString(downloadDirPath),
                                 true, getter_AddRefs(downloadDir));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = NS_ERROR_FAILURE;
    }
#else
  rv = dirService->Get(NS_UNIX_DEFAULT_DOWNLOAD_DIR,
                       NS_GET_IID(nsIFile),
                       getter_AddRefs(downloadDir));
  
  if (NS_FAILED(rv)) {
    rv = dirService->Get(NS_UNIX_HOME_DIR,
                         NS_GET_IID(nsIFile),
                         getter_AddRefs(downloadDir));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = downloadDir->Append(folderName);
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif
#else
  rv = dirService->Get(NS_OS_HOME_DIR,
                       NS_GET_IID(nsIFile),
                       getter_AddRefs(downloadDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = downloadDir->Append(folderName);
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  downloadDir.forget(aResult);

  return NS_OK;
}

#define NS_BRANCH_DOWNLOAD     "browser.download."
#define NS_PREF_FOLDERLIST     "folderList"
#define NS_PREF_DIR            "dir"

NS_IMETHODIMP
nsDownloadManager::GetUserDownloadsDirectory(nsIFile **aResult)
{
  nsresult rv;
  nsCOMPtr<nsIProperties> dirService =
     do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefService> prefService =
     do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefService->GetBranch(NS_BRANCH_DOWNLOAD,
                              getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t val;
  rv = prefBranch->GetIntPref(NS_PREF_FOLDERLIST,
                              &val);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(val) {
    case 0: 
      {
        nsCOMPtr<nsIFile> downloadDir;
        nsCOMPtr<nsIProperties> dirService =
           do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = dirService->Get(NS_OS_DESKTOP_DIR,
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(downloadDir));
        NS_ENSURE_SUCCESS(rv, rv);
        downloadDir.forget(aResult);
        return NS_OK;
      }
      break;
    case 1: 
      return GetDefaultDownloadsDirectory(aResult);
    case 2: 
      {
        nsCOMPtr<nsIFile> customDirectory;
        prefBranch->GetComplexValue(NS_PREF_DIR,
                                    NS_GET_IID(nsIFile),
                                    getter_AddRefs(customDirectory));
        if (customDirectory) {
          bool exists = false;
          (void)customDirectory->Exists(&exists);

          if (!exists) {
            rv = customDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
            if (NS_SUCCEEDED(rv)) {
              customDirectory.forget(aResult);
              return NS_OK;
            }

            
            
          }

          bool writable = false;
          bool directory = false;
          (void)customDirectory->IsWritable(&writable);
          (void)customDirectory->IsDirectory(&directory);

          if (exists && writable && directory) {
            customDirectory.forget(aResult);
            return NS_OK;
          }
        }
        rv = GetDefaultDownloadsDirectory(aResult);
        if (NS_SUCCEEDED(rv)) {
          (void)prefBranch->SetComplexValue(NS_PREF_DIR,
                                            NS_GET_IID(nsIFile),
                                            *aResult);
        }
        return rv;
      }
      break;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsDownloadManager::AddDownload(DownloadType aDownloadType,
                               nsIURI *aSource,
                               nsIURI *aTarget,
                               const nsAString& aDisplayName,
                               nsIMIMEInfo *aMIMEInfo,
                               PRTime aStartTime,
                               nsIFile *aTempFile,
                               nsICancelable *aCancelable,
                               bool aIsPrivate,
                               nsIDownload **aDownload)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aTarget);
  NS_ENSURE_ARG_POINTER(aDownload);

  nsresult rv;

  
  nsCOMPtr<nsIFileURL> targetFileURL = do_QueryInterface(aTarget, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> targetFile;
  rv = targetFileURL->GetFile(getter_AddRefs(targetFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsDownload> dl = new nsDownload();
  if (!dl)
    return NS_ERROR_OUT_OF_MEMORY;

  
  dl->mTarget = aTarget;
  dl->mSource = aSource;
  dl->mTempFile = aTempFile;
  dl->mPrivate = aIsPrivate;

  dl->mDisplayName = aDisplayName;
  if (dl->mDisplayName.IsEmpty())
    targetFile->GetLeafName(dl->mDisplayName);

  dl->mMIMEInfo = aMIMEInfo;
  dl->SetStartTime(aStartTime == 0 ? PR_Now() : aStartTime);

  
  dl->mCancelable = aCancelable;

  
  nsAutoCString source, target;
  aSource->GetSpec(source);
  aTarget->GetSpec(target);

  
  nsAutoString tempPath;
  if (aTempFile)
    aTempFile->GetPath(tempPath);

  
  
  nsAutoCString persistentDescriptor, mimeType;
  nsHandlerInfoAction action = nsIMIMEInfo::saveToDisk;
  if (aMIMEInfo) {
    (void)aMIMEInfo->GetType(mimeType);

    nsCOMPtr<nsIHandlerApp> handlerApp;
    (void)aMIMEInfo->GetPreferredApplicationHandler(getter_AddRefs(handlerApp));
    nsCOMPtr<nsILocalHandlerApp> locHandlerApp = do_QueryInterface(handlerApp);

    if (locHandlerApp) {
      nsCOMPtr<nsIFile> executable;
      (void)locHandlerApp->GetExecutable(getter_AddRefs(executable));
      (void)executable->GetPersistentDescriptor(persistentDescriptor);
    }

    (void)aMIMEInfo->GetPreferredAction(&action);
  }

  int64_t id = AddDownloadToDB(dl->mDisplayName, source, target, tempPath,
                               dl->mStartTime, dl->mLastUpdate,
                               mimeType, persistentDescriptor, action,
                               dl->mPrivate, dl->mGUID );
  NS_ENSURE_TRUE(id, NS_ERROR_FAILURE);
  dl->mID = id;

  rv = AddToCurrentDownloads(dl);
  (void)dl->SetState(nsIDownloadManager::DOWNLOAD_QUEUED);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DOWNLOAD_SCANNER
  if (mScanner) {
    bool scan = true;
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (prefs) {
      (void)prefs->GetBoolPref(PREF_BDM_SCANWHENDONE, &scan);
    }
    
    
    
    
    
    
    if (scan) {
      AVCheckPolicyState res = mScanner->CheckPolicy(aSource, aTarget);
      if (res == AVPOLICY_BLOCKED) {
        
        
        (void)CancelDownload(id);
        (void)dl->SetState(nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY);
      }
    }
  }
#endif

  
  
  
  nsCOMPtr<nsIParentalControlsService> pc =
    do_CreateInstance(NS_PARENTALCONTROLSSERVICE_CONTRACTID);
  if (pc) {
    bool enabled = false;
    (void)pc->GetBlockFileDownloadsEnabled(&enabled);
    if (enabled) {
      (void)CancelDownload(id);
      (void)dl->SetState(nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL);
    }

    
    bool logEnabled = false;
    (void)pc->GetLoggingEnabled(&logEnabled);
    if (logEnabled) {
      (void)pc->Log(nsIParentalControlsService::ePCLog_FileDownload,
                    enabled,
                    aSource,
                    nullptr);
    }
  }

  dl.forget(aDownload);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetDownload(uint32_t aID, nsIDownload **aDownloadItem)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_WARNING("Using integer IDs without compat mode enabled");

  nsDownload *itm = FindDownload(aID);

  nsRefPtr<nsDownload> dl;
  if (!itm) {
    nsresult rv = GetDownloadFromDB(aID, getter_AddRefs(dl));
    NS_ENSURE_SUCCESS(rv, rv);

    itm = dl.get();
  }

  NS_ADDREF(*aDownloadItem = itm);

  return NS_OK;
}

namespace {
class AsyncResult : public nsRunnable
{
public:
  AsyncResult(nsresult aStatus, nsIDownload* aResult,
              nsIDownloadManagerResult* aCallback)
  : mStatus(aStatus), mResult(aResult), mCallback(aCallback)
  {
  }

  NS_IMETHOD Run()
  {
    mCallback->HandleResult(mStatus, mResult);
    return NS_OK;
  }

private:
  nsresult mStatus;
  nsCOMPtr<nsIDownload> mResult;
  nsCOMPtr<nsIDownloadManagerResult> mCallback;
};
} 

NS_IMETHODIMP
nsDownloadManager::GetDownloadByGUID(const nsACString& aGUID,
                                     nsIDownloadManagerResult* aCallback)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  nsDownload *itm = FindDownload(aGUID);

  nsresult rv = NS_OK;
  nsRefPtr<nsDownload> dl;
  if (!itm) {
    rv = GetDownloadFromDB(aGUID, getter_AddRefs(dl));
    itm = dl.get();
  }

  nsRefPtr<AsyncResult> runnable = new AsyncResult(rv, itm, aCallback);
  NS_DispatchToMainThread(runnable);
  return NS_OK;
}

nsDownload *
nsDownloadManager::FindDownload(uint32_t aID)
{
  
  for (int32_t i = mCurrentDownloads.Count() - 1; i >= 0; --i) {
    nsDownload *dl = mCurrentDownloads[i];
    if (dl->mID == aID)
      return dl;
  }

  return nullptr;
}

nsDownload *
nsDownloadManager::FindDownload(const nsACString& aGUID)
{
  
  for (int32_t i = mCurrentDownloads.Count() - 1; i >= 0; --i) {
    nsDownload *dl = mCurrentDownloads[i];
    if (dl->mGUID == aGUID)
      return dl;
  }

  for (int32_t i = mCurrentPrivateDownloads.Count() - 1; i >= 0; --i) {
    nsDownload *dl = mCurrentPrivateDownloads[i];
    if (dl->mGUID == aGUID)
      return dl;
  }

  return nullptr;
}

NS_IMETHODIMP
nsDownloadManager::CancelDownload(uint32_t aID)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_WARNING("Using integer IDs without compat mode enabled");

  
  nsRefPtr<nsDownload> dl = FindDownload(aID);

  
  if (!dl)
    return NS_ERROR_FAILURE;

  return dl->Cancel();
}

nsresult
nsDownloadManager::RetryDownload(const nsACString& aGUID)
{
  nsRefPtr<nsDownload> dl;
  nsresult rv = GetDownloadFromDB(aGUID, getter_AddRefs(dl));
  NS_ENSURE_SUCCESS(rv, rv);

  return RetryDownload(dl);
}


NS_IMETHODIMP
nsDownloadManager::RetryDownload(uint32_t aID)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_WARNING("Using integer IDs without compat mode enabled");

  nsRefPtr<nsDownload> dl;
  nsresult rv = GetDownloadFromDB(aID, getter_AddRefs(dl));
  NS_ENSURE_SUCCESS(rv, rv);

  return RetryDownload(dl);
}

nsresult
nsDownloadManager::RetryDownload(nsDownload* dl)
{
  
  if (dl->mDownloadState != nsIDownloadManager::DOWNLOAD_FAILED &&
      dl->mDownloadState != nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL &&
      dl->mDownloadState != nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY &&
      dl->mDownloadState != nsIDownloadManager::DOWNLOAD_DIRTY &&
      dl->mDownloadState != nsIDownloadManager::DOWNLOAD_CANCELED)
    return NS_ERROR_FAILURE;

  
  nsresult rv;
  if (dl->mDownloadState == nsIDownloadManager::DOWNLOAD_FAILED && dl->IsResumable()) {
    rv = dl->Resume();
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  
  dl->SetStartTime(PR_Now());
  dl->SetProgressBytes(0, -1);

  nsCOMPtr<nsIWebBrowserPersist> wbp =
    do_CreateInstance("@mozilla.org/embedding/browser/nsWebBrowserPersist;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = wbp->SetPersistFlags(nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                            nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddToCurrentDownloads(dl);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dl->SetState(nsIDownloadManager::DOWNLOAD_QUEUED);
  NS_ENSURE_SUCCESS(rv, rv);

  
  dl->mCancelable = wbp;
  (void)wbp->SetProgressListener(dl);

  
  rv = wbp->SavePrivacyAwareURI(dl->mSource, nullptr,
                                nullptr, mozilla::net::RP_Default,
                                nullptr, nullptr,
                                dl->mTarget, dl->mPrivate);
  if (NS_FAILED(rv)) {
    dl->mCancelable = nullptr;
    (void)wbp->SetProgressListener(nullptr);
    return rv;
  }

  return NS_OK;
}

static nsresult
RemoveDownloadByGUID(const nsACString& aGUID, mozIStorageConnection* aDBConn)
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_downloads "
    "WHERE guid = :guid"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), aGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDownloadManager::RemoveDownload(const nsACString& aGUID)
{
  nsRefPtr<nsDownload> dl = FindDownload(aGUID);
  MOZ_ASSERT(!dl, "Can't call RemoveDownload on a download in progress!");
  if (dl)
    return NS_ERROR_FAILURE;

  nsresult rv = GetDownloadFromDB(aGUID, getter_AddRefs(dl));
  NS_ENSURE_SUCCESS(rv, rv);

  if (dl->mPrivate) {
    RemoveDownloadByGUID(aGUID, mPrivateDBConn);
  } else {
    RemoveDownloadByGUID(aGUID, mDBConn);
  }

  return NotifyDownloadRemoval(dl);
}

NS_IMETHODIMP
nsDownloadManager::RemoveDownload(uint32_t aID)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_WARNING("Using integer IDs without compat mode enabled");

  nsRefPtr<nsDownload> dl = FindDownload(aID);
  MOZ_ASSERT(!dl, "Can't call RemoveDownload on a download in progress!");
  if (dl)
    return NS_ERROR_FAILURE;

  nsresult rv = GetDownloadFromDB(aID, getter_AddRefs(dl));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_downloads "
    "WHERE id = :id"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"), aID); 
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  return NotifyDownloadRemoval(dl);
}

nsresult
nsDownloadManager::NotifyDownloadRemoval(nsDownload* aRemoved)
{
  nsCOMPtr<nsISupportsPRUint32> id;
  nsCOMPtr<nsISupportsCString> guid;
  nsresult rv;

  
  bool sendDeprecatedNotification = !(aRemoved && aRemoved->mPrivate);

  if (sendDeprecatedNotification && aRemoved) {
    id = do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    uint32_t dlID;
    rv = aRemoved->GetId(&dlID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = id->SetData(dlID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (sendDeprecatedNotification) {
    mObserverService->NotifyObservers(id,
                                     "download-manager-remove-download",
                                     nullptr);
  }

  if (aRemoved) {
    guid = do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoCString guidStr;
    rv = aRemoved->GetGuid(guidStr);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = guid->SetData(guidStr);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mObserverService->NotifyObservers(guid,
                                    "download-manager-remove-download-guid",
                                    nullptr);
  return NS_OK;
}

static nsresult
DoRemoveDownloadsByTimeframe(mozIStorageConnection* aDBConn,
                             int64_t aStartTime,
                             int64_t aEndTime)
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_downloads "
    "WHERE startTime >= :startTime "
    "AND startTime <= :endTime "
    "AND state NOT IN (:downloading, :paused, :queued)"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("startTime"), aStartTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("endTime"), aEndTime);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("downloading"), nsIDownloadManager::DOWNLOAD_DOWNLOADING);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("paused"), nsIDownloadManager::DOWNLOAD_PAUSED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("queued"), nsIDownloadManager::DOWNLOAD_QUEUED);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::RemoveDownloadsByTimeframe(int64_t aStartTime,
                                              int64_t aEndTime)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  nsresult rv = DoRemoveDownloadsByTimeframe(mDBConn, aStartTime, aEndTime);
  nsresult rv2 = DoRemoveDownloadsByTimeframe(mPrivateDBConn, aStartTime, aEndTime);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(rv2, rv2);

  
  return NotifyDownloadRemoval(nullptr);
}

NS_IMETHODIMP
nsDownloadManager::CleanUp()
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  return CleanUp(mDBConn);
}

NS_IMETHODIMP
nsDownloadManager::CleanUpPrivate()
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  return CleanUp(mPrivateDBConn);
}

nsresult
nsDownloadManager::CleanUp(mozIStorageConnection* aDBConn)
{
  DownloadState states[] = { nsIDownloadManager::DOWNLOAD_FINISHED,
                             nsIDownloadManager::DOWNLOAD_FAILED,
                             nsIDownloadManager::DOWNLOAD_CANCELED,
                             nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL,
                             nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY,
                             nsIDownloadManager::DOWNLOAD_DIRTY };

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_downloads "
    "WHERE state = ? "
      "OR state = ? "
      "OR state = ? "
      "OR state = ? "
      "OR state = ? "
      "OR state = ?"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  for (uint32_t i = 0; i < ArrayLength(states); ++i) {
    rv = stmt->BindInt32ByIndex(i, states[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  return NotifyDownloadRemoval(nullptr);
}

static nsresult
DoGetCanCleanUp(mozIStorageConnection* aDBConn, bool *aResult)
{
  
  
  
  *aResult = false;

  DownloadState states[] = { nsIDownloadManager::DOWNLOAD_FINISHED,
                             nsIDownloadManager::DOWNLOAD_FAILED,
                             nsIDownloadManager::DOWNLOAD_CANCELED,
                             nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL,
                             nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY,
                             nsIDownloadManager::DOWNLOAD_DIRTY };

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT COUNT(*) "
    "FROM moz_downloads "
    "WHERE state = ? "
      "OR state = ? "
      "OR state = ? "
      "OR state = ? "
      "OR state = ? "
      "OR state = ?"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, NS_OK);
  for (uint32_t i = 0; i < ArrayLength(states); ++i) {
    rv = stmt->BindInt32ByIndex(i, states[i]);
    NS_ENSURE_SUCCESS(rv, NS_OK);
  }

  bool moreResults; 
  rv = stmt->ExecuteStep(&moreResults);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  int32_t count;
  rv = stmt->GetInt32(0, &count);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  if (count > 0)
    *aResult = true;

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetCanCleanUp(bool *aResult)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  return DoGetCanCleanUp(mDBConn, aResult);
}

NS_IMETHODIMP
nsDownloadManager::GetCanCleanUpPrivate(bool *aResult)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  return DoGetCanCleanUp(mPrivateDBConn, aResult);
}

NS_IMETHODIMP
nsDownloadManager::PauseDownload(uint32_t aID)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_WARNING("Using integer IDs without compat mode enabled");

  nsDownload *dl = FindDownload(aID);
  if (!dl)
    return NS_ERROR_FAILURE;

  return dl->Pause();
}

NS_IMETHODIMP
nsDownloadManager::ResumeDownload(uint32_t aID)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_WARNING("Using integer IDs without compat mode enabled");

  nsDownload *dl = FindDownload(aID);
  if (!dl)
    return NS_ERROR_FAILURE;

  return dl->Resume();
}

NS_IMETHODIMP
nsDownloadManager::GetDBConnection(mozIStorageConnection **aDBConn)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_ADDREF(*aDBConn = mDBConn);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetPrivateDBConnection(mozIStorageConnection **aDBConn)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  NS_ADDREF(*aDBConn = mPrivateDBConn);

  return NS_OK;
 }

NS_IMETHODIMP
nsDownloadManager::AddListener(nsIDownloadProgressListener *aListener)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  mListeners.AppendObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::AddPrivacyAwareListener(nsIDownloadProgressListener *aListener)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  mPrivacyAwareListeners.AppendObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::RemoveListener(nsIDownloadProgressListener *aListener)
{
  NS_ENSURE_STATE(!mUseJSTransfer);

  mListeners.RemoveObject(aListener);
  mPrivacyAwareListeners.RemoveObject(aListener);
  return NS_OK;
}

void
nsDownloadManager::NotifyListenersOnDownloadStateChange(int16_t aOldState,
                                                        nsDownload *aDownload)
{
  for (int32_t i = mPrivacyAwareListeners.Count() - 1; i >= 0; --i) {
    mPrivacyAwareListeners[i]->OnDownloadStateChange(aOldState, aDownload);
  }

  
  
  if (aDownload->mPrivate) {
    return;
  }

  for (int32_t i = mListeners.Count() - 1; i >= 0; --i) {
    mListeners[i]->OnDownloadStateChange(aOldState, aDownload);
  }
}

void
nsDownloadManager::NotifyListenersOnProgressChange(nsIWebProgress *aProgress,
                                                   nsIRequest *aRequest,
                                                   int64_t aCurSelfProgress,
                                                   int64_t aMaxSelfProgress,
                                                   int64_t aCurTotalProgress,
                                                   int64_t aMaxTotalProgress,
                                                   nsDownload *aDownload)
{
  for (int32_t i = mPrivacyAwareListeners.Count() - 1; i >= 0; --i) {
    mPrivacyAwareListeners[i]->OnProgressChange(aProgress, aRequest, aCurSelfProgress,
                                                aMaxSelfProgress, aCurTotalProgress,
                                                aMaxTotalProgress, aDownload);
  }

  
  
  if (aDownload->mPrivate) {
    return;
  }

  for (int32_t i = mListeners.Count() - 1; i >= 0; --i) {
    mListeners[i]->OnProgressChange(aProgress, aRequest, aCurSelfProgress,
                                    aMaxSelfProgress, aCurTotalProgress,
                                    aMaxTotalProgress, aDownload);
  }
}

void
nsDownloadManager::NotifyListenersOnStateChange(nsIWebProgress *aProgress,
                                                nsIRequest *aRequest,
                                                uint32_t aStateFlags,
                                                nsresult aStatus,
                                                nsDownload *aDownload)
{
  for (int32_t i = mPrivacyAwareListeners.Count() - 1; i >= 0; --i) {
    mPrivacyAwareListeners[i]->OnStateChange(aProgress, aRequest, aStateFlags, aStatus,
                                             aDownload);
  }

  
  
  if (aDownload->mPrivate) {
    return;
  }

  for (int32_t i = mListeners.Count() - 1; i >= 0; --i) {
    mListeners[i]->OnStateChange(aProgress, aRequest, aStateFlags, aStatus,
                                 aDownload);
  }
}




NS_IMETHODIMP
nsDownloadManager::OnBeginUpdateBatch()
{
  
  
  NS_ENSURE_STATE(!mUseJSTransfer);

  
  if (mHistoryTransaction)
    return NS_OK;

  
  mHistoryTransaction = new mozStorageTransaction(mDBConn, true);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnEndUpdateBatch()
{
  
  mHistoryTransaction = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnVisit(nsIURI *aURI, int64_t aVisitID, PRTime aTime,
                           int64_t aSessionID, int64_t aReferringID,
                           uint32_t aTransitionType, const nsACString& aGUID,
                           bool aHidden)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnTitleChanged(nsIURI *aURI,
                                  const nsAString &aPageTitle,
                                  const nsACString &aGUID)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnFrecencyChanged(nsIURI* aURI,
                                     int32_t aNewFrecency,
                                     const nsACString& aGUID,
                                     bool aHidden,
                                     PRTime aLastVisitDate)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnManyFrecenciesChanged()
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnDeleteURI(nsIURI *aURI,
                               const nsACString& aGUID,
                               uint16_t aReason)
{
  
  
  NS_ENSURE_STATE(!mUseJSTransfer);

  nsresult rv = RemoveDownloadsForURI(mGetIdsForURIStatement, aURI);
  nsresult rv2 = RemoveDownloadsForURI(mGetPrivateIdsForURIStatement, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(rv2, rv2);
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnClearHistory()
{
  return CleanUp();
}

NS_IMETHODIMP
nsDownloadManager::OnPageChanged(nsIURI *aURI,
                                 uint32_t aChangedAttribute,
                                 const nsAString& aNewValue,
                                 const nsACString &aGUID)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnDeleteVisits(nsIURI *aURI, PRTime aVisitTime,
                                  const nsACString& aGUID,
                                  uint16_t aReason, uint32_t aTransitionType)
{
  
  return NS_OK;
}




NS_IMETHODIMP
nsDownloadManager::Observe(nsISupports *aSubject,
                           const char *aTopic,
                           const char16_t *aData)
{
  
  
  NS_ENSURE_STATE(!mUseJSTransfer);

  
  
  
  int32_t currDownloadCount = mCurrentDownloads.Count();

  
  
  if (GetQuitBehavior() != QUIT_AND_CANCEL) {
    for (int32_t i = currDownloadCount - 1; i >= 0; --i) {
      if (mCurrentDownloads[i]->IsResumable()) {
        currDownloadCount--;
      }
    }

    
    
    
    currDownloadCount += mCurrentPrivateDownloads.Count();
  }

  nsresult rv;
  if (strcmp(aTopic, "oncancel") == 0) {
    nsCOMPtr<nsIDownload> dl = do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    dl->Cancel();
  } else if (strcmp(aTopic, "profile-before-change") == 0) {
    CloseAllDBs();
  } else if (strcmp(aTopic, "quit-application") == 0) {
    
    
    enum QuitBehavior behavior = GetQuitBehavior();
    if (behavior != QUIT_AND_CANCEL)
      (void)PauseAllDownloads(bool(behavior != QUIT_AND_PAUSE));

    
    (void)RemoveAllDownloads();

   
   
    if (GetRetentionBehavior() == 1)
      CleanUp();
  } else if (strcmp(aTopic, "quit-application-requested") == 0 &&
             currDownloadCount) {
    nsCOMPtr<nsISupportsPRBool> cancelDownloads =
      do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
#ifndef XP_MACOSX
    ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                           MOZ_UTF16("quitCancelDownloadsAlertTitle"),
                           MOZ_UTF16("quitCancelDownloadsAlertMsgMultiple"),
                           MOZ_UTF16("quitCancelDownloadsAlertMsg"),
                           MOZ_UTF16("dontQuitButtonWin"));
#else
    ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                           MOZ_UTF16("quitCancelDownloadsAlertTitle"),
                           MOZ_UTF16("quitCancelDownloadsAlertMsgMacMultiple"),
                           MOZ_UTF16("quitCancelDownloadsAlertMsgMac"),
                           MOZ_UTF16("dontQuitButtonMac"));
#endif
  } else if (strcmp(aTopic, "offline-requested") == 0 && currDownloadCount) {
    nsCOMPtr<nsISupportsPRBool> cancelDownloads =
      do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                           MOZ_UTF16("offlineCancelDownloadsAlertTitle"),
                           MOZ_UTF16("offlineCancelDownloadsAlertMsgMultiple"),
                           MOZ_UTF16("offlineCancelDownloadsAlertMsg"),
                           MOZ_UTF16("dontGoOfflineButton"));
  }
  else if (strcmp(aTopic, NS_IOSERVICE_GOING_OFFLINE_TOPIC) == 0) {
    
    (void)PauseAllDownloads(true);
  }
  else if (strcmp(aTopic, NS_IOSERVICE_OFFLINE_STATUS_TOPIC) == 0 &&
           nsDependentString(aData).EqualsLiteral(NS_IOSERVICE_ONLINE)) {
    
    (void)ResumeAllDownloads(false);
  }
  else if (strcmp(aTopic, "alertclickcallback") == 0) {
    nsCOMPtr<nsIDownloadManagerUI> dmui =
      do_GetService("@mozilla.org/download-manager-ui;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    return dmui->Show(nullptr, nullptr, nsIDownloadManagerUI::REASON_USER_INTERACTED,
                      aData && NS_strcmp(aData, MOZ_UTF16("private")) == 0);
  } else if (strcmp(aTopic, "sleep_notification") == 0 ||
             strcmp(aTopic, "suspend_process_notification") == 0) {
    
    (void)PauseAllDownloads(true);
  } else if (strcmp(aTopic, "wake_notification") == 0 ||
             strcmp(aTopic, "resume_process_notification") == 0) {
    int32_t resumeOnWakeDelay = 10000;
    nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (pref)
      (void)pref->GetIntPref(PREF_BDM_RESUMEONWAKEDELAY, &resumeOnWakeDelay);

    
    
    mResumeOnWakeTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (resumeOnWakeDelay >= 0 && mResumeOnWakeTimer) {
      (void)mResumeOnWakeTimer->InitWithFuncCallback(ResumeOnWakeCallback,
        this, resumeOnWakeDelay, nsITimer::TYPE_ONE_SHOT);
    }
  } else if (strcmp(aTopic, "last-pb-context-exited") == 0) {
    
    
    
    RemoveAllDownloads(mCurrentPrivateDownloads);
    InitPrivateDB();
  } else if (strcmp(aTopic, "last-pb-context-exiting") == 0) {
    
    
    if (!mCurrentPrivateDownloads.Count())
      return NS_OK;

    nsCOMPtr<nsISupportsPRBool> cancelDownloads = do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    ConfirmCancelDownloads(mCurrentPrivateDownloads.Count(), cancelDownloads,
                           MOZ_UTF16("leavePrivateBrowsingCancelDownloadsAlertTitle"),
                           MOZ_UTF16("leavePrivateBrowsingWindowsCancelDownloadsAlertMsgMultiple2"),
                           MOZ_UTF16("leavePrivateBrowsingWindowsCancelDownloadsAlertMsg2"),
                           MOZ_UTF16("dontLeavePrivateBrowsingButton2"));
  }

  return NS_OK;
}

void
nsDownloadManager::ConfirmCancelDownloads(int32_t aCount,
                                          nsISupportsPRBool *aCancelDownloads,
                                          const char16_t *aTitle,
                                          const char16_t *aCancelMessageMultiple,
                                          const char16_t *aCancelMessageSingle,
                                          const char16_t *aDontCancelButton)
{
  
  bool quitRequestCancelled = false;
  aCancelDownloads->GetData(&quitRequestCancelled);
  if (quitRequestCancelled)
    return;

  nsXPIDLString title, message, quitButton, dontQuitButton;

  mBundle->GetStringFromName(aTitle, getter_Copies(title));

  nsAutoString countString;
  countString.AppendInt(aCount);
  const char16_t *strings[1] = { countString.get() };
  if (aCount > 1) {
    mBundle->FormatStringFromName(aCancelMessageMultiple, strings, 1,
                                  getter_Copies(message));
    mBundle->FormatStringFromName(MOZ_UTF16("cancelDownloadsOKTextMultiple"),
                                  strings, 1, getter_Copies(quitButton));
  } else {
    mBundle->GetStringFromName(aCancelMessageSingle, getter_Copies(message));
    mBundle->GetStringFromName(MOZ_UTF16("cancelDownloadsOKText"),
                               getter_Copies(quitButton));
  }

  mBundle->GetStringFromName(aDontCancelButton, getter_Copies(dontQuitButton));

  
  nsCOMPtr<nsIWindowMediator> wm = do_GetService(NS_WINDOWMEDIATOR_CONTRACTID);
  nsCOMPtr<nsIDOMWindow> dmWindow;
  if (wm) {
    wm->GetMostRecentWindow(MOZ_UTF16("Download:Manager"),
                            getter_AddRefs(dmWindow));
  }

  
  nsCOMPtr<nsIPromptService> prompter(do_GetService(NS_PROMPTSERVICE_CONTRACTID));
  if (prompter) {
    int32_t flags = (nsIPromptService::BUTTON_TITLE_IS_STRING * nsIPromptService::BUTTON_POS_0) + (nsIPromptService::BUTTON_TITLE_IS_STRING * nsIPromptService::BUTTON_POS_1);
    bool nothing = false;
    int32_t button;
    prompter->ConfirmEx(dmWindow, title, message, flags, quitButton.get(), dontQuitButton.get(), nullptr, nullptr, &nothing, &button);

    aCancelDownloads->SetData(button == 1);
  }
}




NS_IMPL_CLASSINFO(nsDownload, nullptr, 0, NS_DOWNLOAD_CID)
NS_IMPL_ISUPPORTS_CI(
    nsDownload
  , nsIDownload
  , nsITransfer
  , nsIWebProgressListener
  , nsIWebProgressListener2
)

nsDownload::nsDownload() : mDownloadState(nsIDownloadManager::DOWNLOAD_NOTSTARTED),
                           mID(0),
                           mPercentComplete(0),
                           mCurrBytes(0),
                           mMaxBytes(-1),
                           mStartTime(0),
                           mLastUpdate(PR_Now() - (uint32_t)gUpdateInterval),
                           mResumedAt(-1),
                           mSpeed(0),
                           mHasMultipleFiles(false),
                           mPrivate(false),
                           mAutoResume(DONT_RESUME)
{
}

nsDownload::~nsDownload()
{
}

NS_IMETHODIMP nsDownload::SetSha256Hash(const nsACString& aHash) {
  MOZ_ASSERT(NS_IsMainThread(), "Must call SetSha256Hash on main thread");
  
  mHash = aHash;
  return NS_OK;
}

NS_IMETHODIMP nsDownload::SetSignatureInfo(nsIArray* aSignatureInfo) {
  MOZ_ASSERT(NS_IsMainThread(), "Must call SetSignatureInfo on main thread");
  
  mSignatureInfo = aSignatureInfo;
  return NS_OK;
}

NS_IMETHODIMP nsDownload::SetRedirects(nsIArray* aRedirects) {
  MOZ_ASSERT(NS_IsMainThread(), "Must call SetRedirects on main thread");
  
  mRedirects = aRedirects;
  return NS_OK;
}

#ifdef MOZ_ENABLE_GIO
static void gio_set_metadata_done(GObject *source_obj, GAsyncResult *res, gpointer user_data)
{
  GError *err = nullptr;
  g_file_set_attributes_finish(G_FILE(source_obj), res, nullptr, &err);
  if (err) {
#ifdef DEBUG
    NS_DebugBreak(NS_DEBUG_WARNING, "Set file metadata failed: ", err->message, __FILE__, __LINE__);
#endif
    g_error_free(err);
  }
}
#endif

nsresult
nsDownload::SetState(DownloadState aState)
{
  NS_ASSERTION(mDownloadState != aState,
               "Trying to set the download state to what it already is set to!");

  int16_t oldState = mDownloadState;
  mDownloadState = aState;

  
  nsRefPtr<nsDownload> kungFuDeathGrip = this;

  
  
  
  
  
  
  switch (aState) {
    case nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL:
    case nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY:
    case nsIDownloadManager::DOWNLOAD_DIRTY:
    case nsIDownloadManager::DOWNLOAD_CANCELED:
    case nsIDownloadManager::DOWNLOAD_FAILED:
#ifdef ANDROID
      
      bool tempExists;
      if (mTempFile && NS_SUCCEEDED(mTempFile->Exists(&tempExists)) && tempExists) {
        nsresult rv = mTempFile->Remove(false);
        NS_ENSURE_SUCCESS(rv, rv);
      }
#endif

      
      Finalize();
      break;
#ifdef DOWNLOAD_SCANNER
    case nsIDownloadManager::DOWNLOAD_SCANNING:
    {
      nsresult rv = mDownloadManager->mScanner ? mDownloadManager->mScanner->ScanDownload(this) : NS_ERROR_NOT_INITIALIZED;
      
      if (NS_SUCCEEDED(rv))
        break;
      mDownloadState = aState = nsIDownloadManager::DOWNLOAD_FINISHED;
    }
#endif
    case nsIDownloadManager::DOWNLOAD_FINISHED:
    {
      nsresult rv = ExecuteDesiredAction();
      if (NS_FAILED(rv)) {
        
        
        (void)FailDownload(rv, nullptr);
        return rv;
      }

      
      Finalize();

      nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID));

      
      bool showTaskbarAlert = true;
      if (pref)
        pref->GetBoolPref(PREF_BDM_SHOWALERTONCOMPLETE, &showTaskbarAlert);

      if (showTaskbarAlert) {
        int32_t alertInterval = 2000;
        if (pref)
          pref->GetIntPref(PREF_BDM_SHOWALERTINTERVAL, &alertInterval);

        int64_t alertIntervalUSec = alertInterval * PR_USEC_PER_MSEC;
        int64_t goat = PR_Now() - mStartTime;
        showTaskbarAlert = goat > alertIntervalUSec;

        int32_t size = mPrivate ?
            mDownloadManager->mCurrentPrivateDownloads.Count() :
            mDownloadManager->mCurrentDownloads.Count();
        if (showTaskbarAlert && size == 0) {
          nsCOMPtr<nsIAlertsService> alerts =
            do_GetService("@mozilla.org/alerts-service;1");
          if (alerts) {
              nsXPIDLString title, message;

              mDownloadManager->mBundle->GetStringFromName(
                  MOZ_UTF16("downloadsCompleteTitle"),
                  getter_Copies(title));
              mDownloadManager->mBundle->GetStringFromName(
                  MOZ_UTF16("downloadsCompleteMsg"),
                  getter_Copies(message));

              bool removeWhenDone =
                mDownloadManager->GetRetentionBehavior() == 0;

              
              
              
              
              alerts->ShowAlertNotification(
                  NS_LITERAL_STRING(DOWNLOAD_MANAGER_ALERT_ICON), title,
                  message, !removeWhenDone,
                  mPrivate ? NS_LITERAL_STRING("private") : NS_LITERAL_STRING("non-private"),
                  mDownloadManager, EmptyString(), NS_LITERAL_STRING("auto"),
                  EmptyString(), EmptyString(), nullptr, mPrivate);
            }
        }
      }

#if defined(XP_WIN) || defined(XP_MACOSX) || defined(MOZ_WIDGET_ANDROID) || defined(MOZ_WIDGET_GTK)
      nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(mTarget);
      nsCOMPtr<nsIFile> file;
      nsAutoString path;

      if (fileURL &&
          NS_SUCCEEDED(fileURL->GetFile(getter_AddRefs(file))) &&
          file &&
          NS_SUCCEEDED(file->GetPath(path))) {

#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK) || defined(MOZ_WIDGET_ANDROID)
        
        
        {
          bool addToRecentDocs = true;
          if (pref)
            pref->GetBoolPref(PREF_BDM_ADDTORECENTDOCS, &addToRecentDocs);
#ifdef MOZ_WIDGET_ANDROID
          if (addToRecentDocs) {
            nsCOMPtr<nsIMIMEInfo> mimeInfo;
            nsAutoCString contentType;
            GetMIMEInfo(getter_AddRefs(mimeInfo));

            if (mimeInfo)
              mimeInfo->GetMIMEType(contentType);

            mozilla::widget::DownloadsIntegration::ScanMedia(path, NS_ConvertUTF8toUTF16(contentType));
          }
#else
          if (addToRecentDocs && !mPrivate) {
#ifdef XP_WIN
            ::SHAddToRecentDocs(SHARD_PATHW, path.get());
#elif defined(MOZ_WIDGET_GTK)
            GtkRecentManager* manager = gtk_recent_manager_get_default();

            gchar* uri = g_filename_to_uri(NS_ConvertUTF16toUTF8(path).get(),
                                           nullptr, nullptr);
            if (uri) {
              gtk_recent_manager_add_item(manager, uri);
              g_free(uri);
            }
#endif
          }
#endif
#ifdef MOZ_ENABLE_GIO
          
          GFile* gio_file = g_file_new_for_path(NS_ConvertUTF16toUTF8(path).get());
          nsCString source_uri;
          mSource->GetSpec(source_uri);
          GFileInfo *file_info = g_file_info_new();
          g_file_info_set_attribute_string(file_info, "metadata::download-uri", source_uri.get());
          g_file_set_attributes_async(gio_file,
                                      file_info,
                                      G_FILE_QUERY_INFO_NONE,
                                      G_PRIORITY_DEFAULT,
                                      nullptr, gio_set_metadata_done, nullptr);
          g_object_unref(file_info);
          g_object_unref(gio_file);
#endif
        }
#endif

#ifdef XP_MACOSX
        
        CFStringRef observedObject = ::CFStringCreateWithCString(kCFAllocatorDefault,
                                                 NS_ConvertUTF16toUTF8(path).get(),
                                                 kCFStringEncodingUTF8);
        CFNotificationCenterRef center = ::CFNotificationCenterGetDistributedCenter();
        ::CFNotificationCenterPostNotification(center, CFSTR("com.apple.DownloadFileFinished"),
                                               observedObject, nullptr, TRUE);
        ::CFRelease(observedObject);
#endif
      }

#ifdef XP_WIN
      
      
      
      nsCOMPtr<nsIFile> tempDir, fileDir;
      rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tempDir));
      NS_ENSURE_SUCCESS(rv, rv);
      (void)file->GetParent(getter_AddRefs(fileDir));

      bool isTemp = false;
      if (fileDir)
        (void)fileDir->Equals(tempDir, &isTemp);

      nsCOMPtr<nsILocalFileWin> localFileWin(do_QueryInterface(file));
      if (!isTemp && localFileWin)
        (void)localFileWin->SetFileAttributesWin(nsILocalFileWin::WFA_SEARCH_INDEXED);
#endif

#endif
      
      if (mDownloadManager->GetRetentionBehavior() == 0)
        mDownloadManager->RemoveDownload(mGUID);
    }
    break;
  default:
    break;
  }

  
  
  nsresult rv = UpdateDB();
  NS_ENSURE_SUCCESS(rv, rv);

  mDownloadManager->NotifyListenersOnDownloadStateChange(oldState, this);

  switch (mDownloadState) {
    case nsIDownloadManager::DOWNLOAD_DOWNLOADING:
      
      if (oldState == nsIDownloadManager::DOWNLOAD_QUEUED) {
        if (!mPrivate)
          mDownloadManager->SendEvent(this, "dl-start");
      }
      break;
    case nsIDownloadManager::DOWNLOAD_FAILED:
      if (!mPrivate)
        mDownloadManager->SendEvent(this, "dl-failed");
      break;
    case nsIDownloadManager::DOWNLOAD_SCANNING:
      if (!mPrivate)
        mDownloadManager->SendEvent(this, "dl-scanning");
      break;
    case nsIDownloadManager::DOWNLOAD_FINISHED:
      if (!mPrivate)
        mDownloadManager->SendEvent(this, "dl-done");
      break;
    case nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL:
    case nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY:
      if (!mPrivate)
        mDownloadManager->SendEvent(this, "dl-blocked");
      break;
    case nsIDownloadManager::DOWNLOAD_DIRTY:
      if (!mPrivate)
        mDownloadManager->SendEvent(this, "dl-dirty");
      break;
    case nsIDownloadManager::DOWNLOAD_CANCELED:
      if (!mPrivate)
        mDownloadManager->SendEvent(this, "dl-cancel");
      break;
    default:
      break;
  }
  return NS_OK;
}




NS_IMETHODIMP
nsDownload::OnProgressChange64(nsIWebProgress *aWebProgress,
                               nsIRequest *aRequest,
                               int64_t aCurSelfProgress,
                               int64_t aMaxSelfProgress,
                               int64_t aCurTotalProgress,
                               int64_t aMaxTotalProgress)
{
  if (!mRequest)
    mRequest = aRequest; 

  if (mDownloadState == nsIDownloadManager::DOWNLOAD_QUEUED) {
    
    nsresult rv;
    nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
    nsCOMPtr<nsIURI> referrer = mReferrer;
    if (channel)
      (void)NS_GetReferrerFromChannel(channel, getter_AddRefs(mReferrer));

    
    if (!mReferrer)
      mReferrer = referrer;

    
    
    if (!mMIMEInfo && !mPrivate) {
      nsCOMPtr<nsIDownloadHistory> dh =
        do_GetService(NS_DOWNLOADHISTORY_CONTRACTID);
      if (dh)
        (void)dh->AddDownload(mSource, mReferrer, mStartTime, mTarget);
    }

    
    nsCOMPtr<nsIResumableChannel> resumableChannel(do_QueryInterface(aRequest));
    if (resumableChannel)
      (void)resumableChannel->GetEntityID(mEntityID);

    
    
    
    SetProgressBytes(0, aMaxTotalProgress);

    
    rv = SetState(nsIDownloadManager::DOWNLOAD_DOWNLOADING);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRTime now = PR_Now();
  PRIntervalTime delta = now - mLastUpdate;
  if (delta < gUpdateInterval)
    return NS_OK;

  mLastUpdate = now;

  
  
  double elapsedSecs = double(delta) / PR_USEC_PER_SEC;
  if (elapsedSecs > 0) {
    double speed = double(aCurTotalProgress - mCurrBytes) / elapsedSecs;
    if (mCurrBytes == 0) {
      mSpeed = speed;
    } else {
      
      mSpeed = mSpeed * 0.9 + speed * 0.1;
    }
  }

  SetProgressBytes(aCurTotalProgress, aMaxTotalProgress);

  
  int64_t currBytes, maxBytes;
  (void)GetAmountTransferred(&currBytes);
  (void)GetSize(&maxBytes);
  mDownloadManager->NotifyListenersOnProgressChange(
    aWebProgress, aRequest, currBytes, maxBytes, currBytes, maxBytes, this);

  
  if (aMaxSelfProgress != aMaxTotalProgress)
    mHasMultipleFiles = true;

  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnRefreshAttempted(nsIWebProgress *aWebProgress,
                               nsIURI *aUri,
                               int32_t aDelay,
                               bool aSameUri,
                               bool *allowRefresh)
{
  *allowRefresh = true;
  return NS_OK;
}




NS_IMETHODIMP
nsDownload::OnProgressChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest,
                             int32_t aCurSelfProgress,
                             int32_t aMaxSelfProgress,
                             int32_t aCurTotalProgress,
                             int32_t aMaxTotalProgress)
{
  return OnProgressChange64(aWebProgress, aRequest,
                            aCurSelfProgress, aMaxSelfProgress,
                            aCurTotalProgress, aMaxTotalProgress);
}

NS_IMETHODIMP
nsDownload::OnLocationChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest, nsIURI *aLocation,
                             uint32_t aFlags)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnStatusChange(nsIWebProgress *aWebProgress,
                           nsIRequest *aRequest, nsresult aStatus,
                           const char16_t *aMessage)
{
  if (NS_FAILED(aStatus))
    return FailDownload(aStatus, aMessage);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnStateChange(nsIWebProgress *aWebProgress,
                          nsIRequest *aRequest, uint32_t aStateFlags,
                          nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must call OnStateChange in main thread");

  
  nsRefPtr<nsDownload> kungFuDeathGrip = this;

  
  
  if ((aStateFlags & STATE_START) && (aStateFlags & STATE_IS_NETWORK)) {
    nsresult rv;
    nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(aRequest, &rv);
    if (NS_SUCCEEDED(rv)) {
      uint32_t status;
      rv = channel->GetResponseStatus(&status);
      
      if (NS_SUCCEEDED(rv) && status == 450) {
        
        (void)Cancel();

        
        (void)SetState(nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL);
      }
    }
  } else if ((aStateFlags & STATE_STOP) && (aStateFlags & STATE_IS_NETWORK) &&
             IsFinishable()) {
    
    
    if (NS_SUCCEEDED(aStatus)) {
      
      
      
      
      int64_t fileSize;
      nsCOMPtr<nsIFile> file;
      
      nsCOMPtr<nsIFile> clone;
      if (!mHasMultipleFiles &&
          NS_SUCCEEDED(GetTargetFile(getter_AddRefs(file))) &&
          NS_SUCCEEDED(file->Clone(getter_AddRefs(clone))) &&
          NS_SUCCEEDED(clone->GetFileSize(&fileSize)) && fileSize > 0) {
        mCurrBytes = mMaxBytes = fileSize;

        
        if (WasResumed())
          mResumedAt = 0;
      } else if (mMaxBytes == -1) {
        mMaxBytes = mCurrBytes;
      } else {
        mCurrBytes = mMaxBytes;
      }

      mPercentComplete = 100;
      mLastUpdate = PR_Now();

#ifdef DOWNLOAD_SCANNER
      bool scan = true;
      nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
      if (prefs)
        (void)prefs->GetBoolPref(PREF_BDM_SCANWHENDONE, &scan);

      if (scan)
        (void)SetState(nsIDownloadManager::DOWNLOAD_SCANNING);
      else
        (void)SetState(nsIDownloadManager::DOWNLOAD_FINISHED);
#else
      (void)SetState(nsIDownloadManager::DOWNLOAD_FINISHED);
#endif
    } else {
      
      (void)FailDownload(aStatus, nullptr);
    }
  }

  mDownloadManager->NotifyListenersOnStateChange(aWebProgress, aRequest,
                                                 aStateFlags, aStatus, this);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnSecurityChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest, uint32_t aState)
{
  return NS_OK;
}




NS_IMETHODIMP
nsDownload::Init(nsIURI *aSource,
                 nsIURI *aTarget,
                 const nsAString& aDisplayName,
                 nsIMIMEInfo *aMIMEInfo,
                 PRTime aStartTime,
                 nsIFile *aTempFile,
                 nsICancelable *aCancelable,
                 bool aIsPrivate)
{
  NS_WARNING("Huh...how did we get here?!");
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetState(int16_t *aState)
{
  *aState = mDownloadState;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetDisplayName(nsAString &aDisplayName)
{
  aDisplayName = mDisplayName;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetCancelable(nsICancelable **aCancelable)
{
  *aCancelable = mCancelable;
  NS_IF_ADDREF(*aCancelable);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetTarget(nsIURI **aTarget)
{
  *aTarget = mTarget;
  NS_IF_ADDREF(*aTarget);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetSource(nsIURI **aSource)
{
  *aSource = mSource;
  NS_IF_ADDREF(*aSource);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetStartTime(int64_t *aStartTime)
{
  *aStartTime = mStartTime;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetPercentComplete(int32_t *aPercentComplete)
{
  *aPercentComplete = mPercentComplete;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetAmountTransferred(int64_t *aAmountTransferred)
{
  *aAmountTransferred = mCurrBytes + (WasResumed() ? mResumedAt : 0);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetSize(int64_t *aSize)
{
  *aSize = mMaxBytes + (WasResumed() && mMaxBytes != -1 ? mResumedAt : 0);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetMIMEInfo(nsIMIMEInfo **aMIMEInfo)
{
  *aMIMEInfo = mMIMEInfo;
  NS_IF_ADDREF(*aMIMEInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetTargetFile(nsIFile **aTargetFile)
{
  nsresult rv;

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(mTarget, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  if (NS_FAILED(rv)) {
    return rv;
  }

  file.forget(aTargetFile);
  return rv;
}

NS_IMETHODIMP
nsDownload::GetSpeed(double *aSpeed)
{
  *aSpeed = mSpeed;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetId(uint32_t *aId)
{
  if (mPrivate) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  *aId = mID;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetGuid(nsACString &aGUID)
{
  aGUID = mGUID;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetReferrer(nsIURI **referrer)
{
  NS_IF_ADDREF(*referrer = mReferrer);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetResumable(bool *resumable)
{
  *resumable = IsResumable();
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetIsPrivate(bool *isPrivate)
{
  *isPrivate = mPrivate;
  return NS_OK;
}




void
nsDownload::Finalize()
{
  
  mCancelable = nullptr;

  
  mEntityID.Truncate();
  mTempFile = nullptr;

  
  nsCOMArray<nsDownload>& currentDownloads = mPrivate ?
    mDownloadManager->mCurrentPrivateDownloads :
    mDownloadManager->mCurrentDownloads;
  (void)currentDownloads.RemoveObject(this);

  
  mAutoResume = DONT_RESUME;
}

nsresult
nsDownload::ExecuteDesiredAction()
{
  
  
  
  if (!mTempFile) {
    return NS_OK;
  }

  
  bool fileExists;
  if (NS_FAILED(mTempFile->Exists(&fileExists)) || !fileExists)
    return NS_ERROR_FILE_NOT_FOUND;

  
  nsHandlerInfoAction action = nsIMIMEInfo::saveToDisk;
  if (mMIMEInfo) {
    nsresult rv = mMIMEInfo->GetPreferredAction(&action);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsresult rv = NS_OK;
  switch (action) {
    case nsIMIMEInfo::saveToDisk:
      
      rv = MoveTempToTarget();
      if (NS_SUCCEEDED(rv)) {
        rv = FixTargetPermissions();
      }
      break;
    case nsIMIMEInfo::useHelperApp:
    case nsIMIMEInfo::useSystemDefault:
      
      
      rv = OpenWithApplication();
      break;
    default:
      break;
  }

  return rv;
}

nsresult
nsDownload::FixTargetPermissions()
{
  nsCOMPtr<nsIFile> target;
  nsresult rv = GetTargetFile(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPropertyBag2> infoService =
      do_GetService("@mozilla.org/system-info;1");
  uint32_t gUserUmask = 0;
  rv = infoService->GetPropertyAsUint32(NS_LITERAL_STRING("umask"),
                                        &gUserUmask);
  if (NS_SUCCEEDED(rv)) {
    (void)target->SetPermissions(0666 & ~gUserUmask);
  }
  return NS_OK;
}

nsresult
nsDownload::MoveTempToTarget()
{
  nsCOMPtr<nsIFile> target;
  nsresult rv = GetTargetFile(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  bool fileExists;
  if (NS_SUCCEEDED(target->Exists(&fileExists)) && fileExists) {
    rv = target->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsAutoString fileName;
  rv = target->GetLeafName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFile> dir;
  rv = target->GetParent(getter_AddRefs(dir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTempFile->MoveTo(dir, fileName);
  return rv;
}

nsresult
nsDownload::OpenWithApplication()
{
  
  nsCOMPtr<nsIFile> target;
  nsresult rv = GetTargetFile(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = MoveTempToTarget();
  NS_ENSURE_SUCCESS(rv, rv);

  bool deleteTempFileOnExit;
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefs || NS_FAILED(prefs->GetBoolPref(PREF_BH_DELETETEMPFILEONEXIT,
                                             &deleteTempFileOnExit))) {
    
#if !defined(XP_MACOSX)
    
    
    
    deleteTempFileOnExit = true;
#else
    deleteTempFileOnExit = false;
#endif
  }

  
  
  if (deleteTempFileOnExit || mPrivate) {

    
    target->SetPermissions(0400);

    
    
    nsCOMPtr<nsPIExternalAppLauncher> appLauncher(do_GetService
                    (NS_EXTERNALHELPERAPPSERVICE_CONTRACTID));

    
    
    if (appLauncher) {
      if (mPrivate) {
        (void)appLauncher->DeleteTemporaryPrivateFileWhenPossible(target);
      } else {
        (void)appLauncher->DeleteTemporaryFileOnExit(target);
      }
    }
  }

  return mMIMEInfo->LaunchWithFile(target);
}

void
nsDownload::SetStartTime(int64_t aStartTime)
{
  mStartTime = aStartTime;
  mLastUpdate = aStartTime;
}

void
nsDownload::SetProgressBytes(int64_t aCurrBytes, int64_t aMaxBytes)
{
  mCurrBytes = aCurrBytes;
  mMaxBytes = aMaxBytes;

  
  int64_t currBytes, maxBytes;
  (void)GetAmountTransferred(&currBytes);
  (void)GetSize(&maxBytes);

  if (currBytes == maxBytes)
    mPercentComplete = 100;
  else if (maxBytes <= 0)
    mPercentComplete = -1;
  else
    mPercentComplete = (int32_t)((double)currBytes / maxBytes * 100 + .5);
}

NS_IMETHODIMP
nsDownload::Pause()
{
  if (!IsResumable())
    return NS_ERROR_UNEXPECTED;

  nsresult rv = CancelTransfer();
  NS_ENSURE_SUCCESS(rv, rv);

  return SetState(nsIDownloadManager::DOWNLOAD_PAUSED);
}

nsresult
nsDownload::CancelTransfer()
{
  nsresult rv = NS_OK;
  if (mCancelable) {
    rv = mCancelable->Cancel(NS_BINDING_ABORTED);
    
    mCancelable = nullptr;
  }

  return rv;
}

NS_IMETHODIMP
nsDownload::Cancel()
{
  
  if (IsFinished())
    return NS_OK;

  
  (void)CancelTransfer();

  
  
  
  
  if (mTempFile) {
    bool exists;
    mTempFile->Exists(&exists);
    if (exists)
      mTempFile->Remove(false);
  }

  nsCOMPtr<nsIFile> file;
  if (NS_SUCCEEDED(GetTargetFile(getter_AddRefs(file))))
  {
    bool exists;
    file->Exists(&exists);
    if (exists)
      file->Remove(false);
  }

  nsresult rv = SetState(nsIDownloadManager::DOWNLOAD_CANCELED);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDownload::Resume()
{
  if (!IsPaused() || !IsResumable())
    return NS_ERROR_UNEXPECTED;

  nsresult rv;
  nsCOMPtr<nsIWebBrowserPersist> wbp =
    do_CreateInstance("@mozilla.org/embedding/browser/nsWebBrowserPersist;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = wbp->SetPersistFlags(nsIWebBrowserPersist::PERSIST_FLAGS_APPEND_TO_FILE |
                            nsIWebBrowserPersist::PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIChannel> channel;
  nsCOMPtr<nsIInterfaceRequestor> ir(do_QueryInterface(wbp));
  rv = NS_NewChannel(getter_AddRefs(channel),
                     mSource,
                     nsContentUtils::GetSystemPrincipal(),
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_OTHER,
                     nullptr,  
                     ir);

  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateBrowsingChannel> pbChannel = do_QueryInterface(channel);
  if (pbChannel) {
    pbChannel->SetPrivate(mPrivate);
  }

  
  
  nsCOMPtr<nsIFile> targetLocalFile(mTempFile);
  if (!targetLocalFile) {
    rv = GetTargetFile(getter_AddRefs(targetLocalFile));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  int64_t fileSize;
  
  nsCOMPtr<nsIFile> clone;
  if (NS_FAILED(targetLocalFile->Clone(getter_AddRefs(clone))) ||
      NS_FAILED(clone->GetFileSize(&fileSize)))
    fileSize = 0;

  
  nsCOMPtr<nsIResumableChannel> resumableChannel(do_QueryInterface(channel));
  if (!resumableChannel)
    return NS_ERROR_UNEXPECTED;
  rv = resumableChannel->ResumeAt(fileSize, mEntityID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t maxBytes;
  GetSize(&maxBytes);
  SetProgressBytes(0, maxBytes != -1 ? maxBytes - fileSize : -1);
  
  mResumedAt = fileSize;

  
  if (mReferrer) {
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
    if (httpChannel) {
      rv = httpChannel->SetReferrer(mReferrer);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  mCancelable = wbp;
  (void)wbp->SetProgressListener(this);

  
  rv = wbp->SaveChannel(channel, targetLocalFile);
  if (NS_FAILED(rv)) {
    mCancelable = nullptr;
    (void)wbp->SetProgressListener(nullptr);
    return rv;
  }

  return SetState(nsIDownloadManager::DOWNLOAD_DOWNLOADING);
}

NS_IMETHODIMP
nsDownload::Remove()
{
  return mDownloadManager->RemoveDownload(mGUID);
}

NS_IMETHODIMP
nsDownload::Retry()
{
  return mDownloadManager->RetryDownload(mGUID);
}

bool
nsDownload::IsPaused()
{
  return mDownloadState == nsIDownloadManager::DOWNLOAD_PAUSED;
}

bool
nsDownload::IsResumable()
{
  return !mEntityID.IsEmpty();
}

bool
nsDownload::WasResumed()
{
  return mResumedAt != -1;
}

bool
nsDownload::ShouldAutoResume()
{
  return mAutoResume == AUTO_RESUME;
}

bool
nsDownload::IsFinishable()
{
  return mDownloadState == nsIDownloadManager::DOWNLOAD_NOTSTARTED ||
         mDownloadState == nsIDownloadManager::DOWNLOAD_QUEUED ||
         mDownloadState == nsIDownloadManager::DOWNLOAD_DOWNLOADING;
}

bool
nsDownload::IsFinished()
{
  return mDownloadState == nsIDownloadManager::DOWNLOAD_FINISHED;
}

nsresult
nsDownload::UpdateDB()
{
  NS_ASSERTION(mID, "Download ID is stored as zero.  This is bad!");
  NS_ASSERTION(mDownloadManager, "Egads!  We have no download manager!");

  mozIStorageStatement *stmt = mPrivate ?
    mDownloadManager->mUpdatePrivateDownloadStatement : mDownloadManager->mUpdateDownloadStatement;

  nsAutoString tempPath;
  if (mTempFile)
    (void)mTempFile->GetPath(tempPath);
  nsresult rv = stmt->BindStringByName(NS_LITERAL_CSTRING("tempPath"), tempPath);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("startTime"), mStartTime);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("endTime"), mLastUpdate);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("state"), mDownloadState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mReferrer) {
    nsAutoCString referrer;
    rv = mReferrer->GetSpec(referrer);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("referrer"), referrer);
  } else {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("referrer"));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("entityID"), mEntityID);
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t currBytes;
  (void)GetAmountTransferred(&currBytes);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("currBytes"), currBytes);
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t maxBytes;
  (void)GetSize(&maxBytes);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("maxBytes"), maxBytes);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("autoResume"), mAutoResume);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"), mID);
  NS_ENSURE_SUCCESS(rv, rv);

  return stmt->Execute();
}

nsresult
nsDownload::FailDownload(nsresult aStatus, const char16_t *aMessage)
{
  
  nsCOMPtr<nsIStringBundle> bundle = mDownloadManager->mBundle;

  (void)SetState(nsIDownloadManager::DOWNLOAD_FAILED);

  
  nsXPIDLString title;
  nsresult rv = bundle->GetStringFromName(
    MOZ_UTF16("downloadErrorAlertTitle"), getter_Copies(title));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsXPIDLString message;
  message = aMessage;
  if (message.IsEmpty()) {
    rv = bundle->GetStringFromName(
      MOZ_UTF16("downloadErrorGeneric"), getter_Copies(message));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIWindowMediator> wm =
    do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMWindow> dmWindow;
  rv = wm->GetMostRecentWindow(MOZ_UTF16("Download:Manager"),
                               getter_AddRefs(dmWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPromptService> prompter =
    do_GetService("@mozilla.org/embedcomp/prompt-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return prompter->Alert(dmWindow, title, message);
}
