











































#include "mozIStorageService.h"
#include "nsIAlertsService.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDownloadHistory.h"
#include "nsIDownloadManagerUI.h"
#include "nsIMIMEService.h"
#include "nsIParentalControlsService.h"
#include "nsIPrefService.h"
#include "nsIPrivateBrowsingService.h"
#include "nsIPromptService.h"
#include "nsIResumableChannel.h"
#include "nsIWebBrowserPersist.h"
#include "nsIWindowMediator.h"
#include "nsILocalFileWin.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsArrayEnumerator.h"
#include "nsCExternalHandlerService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDownloadManager.h"
#include "nsNetUtil.h"

#include "mozStorageCID.h"
#include "nsDocShellCID.h"
#include "nsEmbedCID.h"
#include "nsToolkitCompsCID.h"

#if defined(XP_WIN) && !defined(WINCE)
#include <shlobj.h>
#ifdef DOWNLOAD_SCANNER
#include "nsDownloadScanner.h"
#endif
#endif

#define DOWNLOAD_MANAGER_BUNDLE "chrome://mozapps/locale/downloads/downloads.properties"
#define DOWNLOAD_MANAGER_ALERT_ICON "chrome://mozapps/skin/downloads/downloadIcon.png"
#define PREF_BDM_SHOWALERTONCOMPLETE "browser.download.manager.showAlertOnComplete"
#define PREF_BDM_SHOWALERTINTERVAL "browser.download.manager.showAlertInterval"
#define PREF_BDM_RETENTION "browser.download.manager.retention"
#define PREF_BDM_QUITBEHAVIOR "browser.download.manager.quitBehavior"
#define PREF_BDM_ADDTORECENTDOCS "browser.download.manager.addToRecentDocs"
#define PREF_BDM_SCANWHENDONE "browser.download.manager.scanWhenDone"
#define PREF_BDM_RESUMEONWAKEDELAY "browser.download.manager.resumeOnWakeDelay"
#define PREF_BH_DELETETEMPFILEONEXIT "browser.helperApps.deleteTempFileOnExit"

static const PRInt64 gUpdateInterval = 400 * PR_USEC_PER_MSEC;

#define DM_SCHEMA_VERSION      8
#define DM_DB_NAME             NS_LITERAL_STRING("downloads.sqlite")
#define DM_DB_CORRUPT_FILENAME NS_LITERAL_STRING("downloads.sqlite.corrupt")

#define NS_SYSTEMINFO_CONTRACTID "@mozilla.org/system-info;1"




NS_IMPL_ISUPPORTS3(
  nsDownloadManager
, nsIDownloadManager
, nsINavHistoryObserver
, nsIObserver
)

nsDownloadManager *nsDownloadManager::gDownloadManagerService = nsnull;

nsDownloadManager *
nsDownloadManager::GetSingleton()
{
  if (gDownloadManagerService) {
    NS_ADDREF(gDownloadManagerService);
    return gDownloadManagerService;
  }

  gDownloadManagerService = new nsDownloadManager();
  if (gDownloadManagerService) {
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
    mScanner = nsnull;
  }
#endif
  gDownloadManagerService = nsnull;
}

nsresult
nsDownloadManager::ResumeRetry(nsDownload *aDl)
{
  
  nsRefPtr<nsDownload> dl = aDl;

  
  nsresult rv = dl->Resume();

  
  if (NS_FAILED(rv)) {
    
    rv = CancelDownload(dl->mID);

    
    if (NS_SUCCEEDED(rv))
      rv = RetryDownload(dl->mID);
  }

  return rv;
}

nsresult
nsDownloadManager::PauseAllDownloads(PRBool aSetResume)
{
  nsresult retVal = NS_OK;
  for (PRInt32 i = mCurrentDownloads.Count() - 1; i >= 0; --i) {
    nsRefPtr<nsDownload> dl = mCurrentDownloads[i];

    
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
nsDownloadManager::ResumeAllDownloads(PRBool aResumeAll)
{
  nsresult retVal = NS_OK;
  for (PRInt32 i = mCurrentDownloads.Count() - 1; i >= 0; --i) {
    nsRefPtr<nsDownload> dl = mCurrentDownloads[i];

    
    
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
  nsresult rv = NS_OK;
  for (PRInt32 i = mCurrentDownloads.Count() - 1; i >= 0; --i) {
    nsRefPtr<nsDownload> dl = mCurrentDownloads[0];

    nsresult result;
    if (dl->IsPaused() && GetQuitBehavior() != QUIT_AND_CANCEL)
      result = mCurrentDownloads.RemoveObject(dl);
    else
      result = CancelDownload(dl->mID);

    
    if (NS_FAILED(result))
      rv = result;
  }

  return rv;
}

nsresult
nsDownloadManager::RemoveDownloadsForURI(nsIURI *aURI)
{
  mozStorageStatementScoper scope(mGetIdsForURIStatement);

  nsCAutoString source;
  nsresult rv = aURI->GetSpec(source);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mGetIdsForURIStatement->BindUTF8StringParameter(0, source);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  nsAutoTArray<PRInt64, 4> downloads;
  
  while (NS_SUCCEEDED(mGetIdsForURIStatement->ExecuteStep(&hasMore)) &&
         hasMore) {
    PRInt64 downloadId;
    rv = mGetIdsForURIStatement->GetInt64(0, &downloadId);
    NS_ENSURE_SUCCESS(rv, rv);

    downloads.AppendElement(downloadId);
  }

  
  for (PRInt32 i = downloads.Length(); --i >= 0; )
    (void)RemoveDownload(downloads[i]);

  return NS_OK;
}

void 
nsDownloadManager::ResumeOnWakeCallback(nsITimer *aTimer, void *aClosure)
{
  
  nsDownloadManager *dlMgr = static_cast<nsDownloadManager *>(aClosure);
  (void)dlMgr->ResumeAllDownloads(PR_FALSE);
}

already_AddRefed<mozIStorageConnection>
nsDownloadManager::GetFileDBConnection(nsIFile *dbFile) const
{
  NS_ASSERTION(dbFile, "GetFileDBConnection called with an invalid nsIFile");

  nsCOMPtr<mozIStorageService> storage =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(storage, nsnull);

  nsCOMPtr<mozIStorageConnection> conn;
  nsresult rv = storage->OpenDatabase(dbFile, getter_AddRefs(conn));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    
    rv = dbFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, nsnull);
    rv = storage->OpenDatabase(dbFile, getter_AddRefs(conn));
  }
  NS_ENSURE_SUCCESS(rv, nsnull);

  return conn.forget();
}

already_AddRefed<mozIStorageConnection>
nsDownloadManager::GetMemoryDBConnection() const
{
  nsCOMPtr<mozIStorageService> storage =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(storage, nsnull);

  nsCOMPtr<mozIStorageConnection> conn;
  nsresult rv = storage->OpenSpecialDatabase("memory", getter_AddRefs(conn));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return conn.forget();
}

nsresult
nsDownloadManager::InitMemoryDB()
{
  mDBConn = GetMemoryDBConnection();
  if (!mDBConn)
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = CreateTable();
  NS_ENSURE_SUCCESS(rv, rv);

  mDBType = DATABASE_MEMORY;
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

  mDBConn = GetFileDBConnection(dbFile);
  NS_ENSURE_TRUE(mDBConn, NS_ERROR_NOT_AVAILABLE);

  PRBool tableExists;
  rv = mDBConn->TableExists(NS_LITERAL_CSTRING("moz_downloads"), &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!tableExists) {
    rv = CreateTable();
    NS_ENSURE_SUCCESS(rv, rv);
    mDBType = DATABASE_DISK;
    return NS_OK;
  }

  mDBType = DATABASE_DISK;

  
  PRInt32 schemaVersion;
  rv = mDBConn->GetSchemaVersion(&schemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  

  switch (schemaVersion) {
  
  
  
  
  case 1: 
    {
      
      mozStorageTransaction safeTransaction(mDBConn, PR_TRUE);

      
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
               "preferredApplication, preferredAction, autoResume "
        "FROM moz_downloads"), getter_AddRefs(stmt));
      if (NS_SUCCEEDED(rv))
        break;

      
      
      nsCOMPtr<mozIStorageService> storage =
        do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
      NS_ENSURE_TRUE(storage, NS_ERROR_NOT_AVAILABLE);
      nsCOMPtr<nsIFile> backup;
      rv = storage->BackupDatabaseFile(dbFile, DM_DB_CORRUPT_FILENAME, nsnull,
                                       getter_AddRefs(backup));
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP TABLE moz_downloads"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = CreateTable();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;
  }

  return NS_OK;
}

nsresult
nsDownloadManager::CreateTable()
{
  nsresult rv = mDBConn->SetSchemaVersion(DM_SCHEMA_VERSION);
  if (NS_FAILED(rv)) return rv;

  return mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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
      "autoResume INTEGER NOT NULL DEFAULT 0"
    ")"));
}

nsresult
nsDownloadManager::RestoreDatabaseState()
{
  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_downloads "
    "SET state = ?1 "
    "WHERE state = ?2"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 i = 0;
  rv = stmt->BindInt32Parameter(i++, nsIDownloadManager::DOWNLOAD_FINISHED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(i++, nsIDownloadManager::DOWNLOAD_SCANNING);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_downloads "
    "SET autoResume = ?1 "
    "WHERE state = ?2 "
      "OR state = ?3 "
      "OR state = ?4"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  i = 0;
  rv = stmt->BindInt32Parameter(i++, nsDownload::AUTO_RESUME);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(i++, nsIDownloadManager::DOWNLOAD_NOTSTARTED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(i++, nsIDownloadManager::DOWNLOAD_QUEUED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(i++, nsIDownloadManager::DOWNLOAD_DOWNLOADING);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_downloads "
    "SET autoResume = ?1 "
    "WHERE state = ?2 "
      "AND autoResume = ?3"),
    getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  i = 0;
  rv = stmt->BindInt32Parameter(i++, nsDownload::DONT_RESUME);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(i++, nsIDownloadManager::DOWNLOAD_FINISHED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(i++, nsDownload::AUTO_RESUME);
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
    "WHERE (state = ?1 AND LENGTH(entityID) > 0) "
      "OR autoResume != ?2"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32Parameter(0, nsIDownloadManager::DOWNLOAD_PAUSED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(1, nsDownload::DONT_RESUME);
  NS_ENSURE_SUCCESS(rv, rv);

  nsresult retVal = NS_OK;
  PRBool hasResults;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResults)) && hasResults) {
    nsRefPtr<nsDownload> dl;
    
    
    
    if (NS_FAILED(GetDownloadFromDB(stmt->AsInt32(0), getter_AddRefs(dl))) ||
        NS_FAILED(AddToCurrentDownloads(dl)))
      retVal = NS_ERROR_FAILURE;
  }

  
  rv = ResumeAllDownloads(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return retVal;
}

PRInt64
nsDownloadManager::AddDownloadToDB(const nsAString &aName,
                                   const nsACString &aSource,
                                   const nsACString &aTarget,
                                   const nsAString &aTempPath,
                                   PRInt64 aStartTime,
                                   PRInt64 aEndTime,
                                   const nsACString &aMimeType,
                                   const nsACString &aPreferredApp,
                                   nsHandlerInfoAction aPreferredAction)
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_downloads "
    "(name, source, target, tempPath, startTime, endTime, state, "
     "mimeType, preferredApplication, preferredAction) "
    "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, 0);

  PRInt32 i = 0;
  
  rv = stmt->BindStringParameter(i++, aName);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindUTF8StringParameter(i++, aSource);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindUTF8StringParameter(i++, aTarget);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindStringParameter(i++, aTempPath);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindInt64Parameter(i++, aStartTime);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindInt64Parameter(i++, aEndTime);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindInt32Parameter(i++, nsIDownloadManager::DOWNLOAD_NOTSTARTED);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindUTF8StringParameter(i++, aMimeType);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindUTF8StringParameter(i++, aPreferredApp);
  NS_ENSURE_SUCCESS(rv, 0);

  
  rv = stmt->BindInt32Parameter(i++, aPreferredAction);
  NS_ENSURE_SUCCESS(rv, 0);

  PRBool hasMore;
  rv = stmt->ExecuteStep(&hasMore); 
  NS_ENSURE_SUCCESS(rv, 0);

  PRInt64 id = 0;
  rv = mDBConn->GetLastInsertRowID(&id);
  NS_ENSURE_SUCCESS(rv, 0);

  
  return id;
}

nsresult
nsDownloadManager::InitDB()
{
  nsresult rv = NS_OK;

  switch (mDBType) {
    case DATABASE_MEMORY:
      rv = InitMemoryDB();
      break;

    case DATABASE_DISK:
      rv = InitFileDB();
      break;

    default:
      NS_ERROR("Unexpected value encountered for nsDownloadManager::mDBType");
      break;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_downloads "
    "SET tempPath = ?1, startTime = ?2, endTime = ?3, state = ?4, "
        "referrer = ?5, entityID = ?6, currBytes = ?7, maxBytes = ?8, "
        "autoResume = ?9 "
    "WHERE id = ?10"), getter_AddRefs(mUpdateDownloadStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id "
    "FROM moz_downloads "
    "WHERE source = ?1"), getter_AddRefs(mGetIdsForURIStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

nsresult
nsDownloadManager::Init()
{
  
  {
    nsCOMPtr<nsIFile> oldDownloadsFile;
    PRBool fileExists;
    if (NS_SUCCEEDED(NS_GetSpecialDirectory(NS_APP_DOWNLOADS_50_FILE,
          getter_AddRefs(oldDownloadsFile))) &&
        NS_SUCCEEDED(oldDownloadsFile->Exists(&fileExists)) &&
        fileExists) {
      (void)oldDownloadsFile->Remove(PR_FALSE);
    }
  }

  nsresult rv;
  mObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = bundleService->CreateBundle(DOWNLOAD_MANAGER_BUNDLE,
                                   getter_AddRefs(mBundle));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DOWNLOAD_SCANNER
  mScanner = new nsDownloadScanner();
  if (!mScanner)
    return NS_ERROR_OUT_OF_MEMORY;
  rv = mScanner->Init();
  if (NS_FAILED(rv)) {
    delete mScanner;
    mScanner = nsnull;
  }
#endif

  
  
  rv = RestoreDatabaseState();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RestoreActiveDownloads();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to restore all active downloads");

  nsCOMPtr<nsIPrivateBrowsingService> pbs =
    do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (pbs) {
    (void)pbs->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);
    if (mInPrivateBrowsing)
      OnEnterPrivateBrowsingMode();
  }

  nsCOMPtr<nsINavHistoryService> history =
    do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);

  
  
  
  
  
  
  
  
  
  mObserverService->AddObserver(this, "quit-application", PR_FALSE);
  mObserverService->AddObserver(this, "quit-application-requested", PR_FALSE);
  mObserverService->AddObserver(this, "offline-requested", PR_FALSE);
  mObserverService->AddObserver(this, "sleep_notification", PR_FALSE);
  mObserverService->AddObserver(this, "wake_notification", PR_FALSE);
  mObserverService->AddObserver(this, NS_IOSERVICE_GOING_OFFLINE_TOPIC, PR_FALSE);
  mObserverService->AddObserver(this, NS_IOSERVICE_OFFLINE_STATUS_TOPIC, PR_FALSE);
  mObserverService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_FALSE);

  if (history)
    (void)history->AddObserver(this, PR_FALSE);

  return NS_OK;
}

PRInt32
nsDownloadManager::GetRetentionBehavior()
{
  
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, 0);

  PRInt32 val;
  rv = pref->GetIntPref(PREF_BDM_RETENTION, &val);
  NS_ENSURE_SUCCESS(rv, 0);

  return val;
}

enum nsDownloadManager::QuitBehavior
nsDownloadManager::GetQuitBehavior()
{
  
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, QUIT_AND_RESUME);

  PRInt32 val;
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
nsDownloadManager::GetDownloadFromDB(PRUint32 aID, nsDownload **retVal)
{
  NS_ASSERTION(!FindDownload(aID),
               "If it is a current download, you should not call this method!");

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, state, startTime, source, target, tempPath, name, referrer, "
           "entityID, currBytes, maxBytes, mimeType, preferredAction, "
           "preferredApplication, autoResume "
    "FROM moz_downloads "
    "WHERE id = ?1"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt64Parameter(0, aID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResults = PR_FALSE;
  rv = stmt->ExecuteStep(&hasResults);
  if (NS_FAILED(rv) || !hasResults)
    return NS_ERROR_NOT_AVAILABLE;

  
  nsRefPtr<nsDownload> dl = new nsDownload();
  if (!dl)
    return NS_ERROR_OUT_OF_MEMORY;

  PRInt32 i = 0;
  
  dl->mCancelable = nsnull;
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
    rv = NS_NewLocalFile(tempPath, PR_TRUE, getter_AddRefs(dl->mTempFile));
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

  PRInt64 currBytes = stmt->AsInt64(i++);
  PRInt64 maxBytes = stmt->AsInt64(i++);
  dl->SetProgressBytes(currBytes, maxBytes);

  
  nsCAutoString mimeType;
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

    nsCAutoString persistentDescriptor;
    rv = stmt->GetUTF8String(i++, persistentDescriptor);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!persistentDescriptor.IsEmpty()) {
      nsCOMPtr<nsILocalHandlerApp> handler =
        do_CreateInstance(NS_LOCALHANDLERAPP_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsILocalFile> localExecutable;
      rv = NS_NewNativeLocalFile(EmptyCString(), PR_FALSE,
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

  
  NS_ADDREF(*retVal = dl);
  return NS_OK;
}

nsresult
nsDownloadManager::AddToCurrentDownloads(nsDownload *aDl)
{
  if (!mCurrentDownloads.AppendObject(aDl))
    return NS_ERROR_OUT_OF_MEMORY;

  aDl->mDownloadManager = this;
  return NS_OK;
}

void
nsDownloadManager::SendEvent(nsDownload *aDownload, const char *aTopic)
{
  (void)mObserverService->NotifyObservers(aDownload, aTopic, nsnull);
}




NS_IMETHODIMP
nsDownloadManager::GetActiveDownloadCount(PRInt32 *aResult)
{
  *aResult = mCurrentDownloads.Count();

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetActiveDownloads(nsISimpleEnumerator **aResult)
{
  return NS_NewArrayEnumerator(aResult, mCurrentDownloads);
}

NS_IMETHODIMP
nsDownloadManager::GetDefaultDownloadsDirectory(nsILocalFile **aResult)
{
  nsCOMPtr<nsILocalFile> downloadDir;

  nsresult rv;
  nsCOMPtr<nsIProperties> dirService =
     do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  

  nsXPIDLString folderName;
  mBundle->GetStringFromName(NS_LITERAL_STRING("downloadsFolder").get(),
                             getter_Copies(folderName));

#if defined (XP_MACOSX)
  rv = dirService->Get(NS_OSX_DEFAULT_DOWNLOAD_DIR,
                       NS_GET_IID(nsILocalFile),
                       getter_AddRefs(downloadDir));
  NS_ENSURE_SUCCESS(rv, rv);
#elif defined(XP_WIN) && !defined(WINCE)
  rv = dirService->Get(NS_WIN_DEFAULT_DOWNLOAD_DIR,
                       NS_GET_IID(nsILocalFile),
                       getter_AddRefs(downloadDir));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPropertyBag2> infoService =
     do_GetService(NS_SYSTEMINFO_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 version;
  NS_NAMED_LITERAL_STRING(osVersion, "version");
  rv = infoService->GetPropertyAsInt32(osVersion, &version);
  NS_ENSURE_SUCCESS(rv, rv);
  if (version < 6) { 
    
    rv = dirService->Get(NS_WIN_PERSONAL_DIR,
                         NS_GET_IID(nsILocalFile),
                         getter_AddRefs(downloadDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = downloadDir->Append(folderName);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    PRBool exists;
    rv = downloadDir->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!exists) {
      rv = downloadDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
#elif defined(XP_UNIX)
#if defined(NS_OSSO)
    
    
    
    
    
    rv = dirService->Get(NS_UNIX_XDG_DOCUMENTS_DIR,
                         NS_GET_IID(nsILocalFile),
                         getter_AddRefs(downloadDir));
#else
  rv = dirService->Get(NS_UNIX_DEFAULT_DOWNLOAD_DIR,
                       NS_GET_IID(nsILocalFile),
                       getter_AddRefs(downloadDir));
  
  if (NS_FAILED(rv)) {
    rv = dirService->Get(NS_UNIX_HOME_DIR,
                         NS_GET_IID(nsILocalFile),
                         getter_AddRefs(downloadDir));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = downloadDir->Append(folderName);
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif
#else
  rv = dirService->Get(NS_OS_HOME_DIR,
                       NS_GET_IID(nsILocalFile),
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
nsDownloadManager::GetUserDownloadsDirectory(nsILocalFile **aResult)
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

  PRInt32 val;
  rv = prefBranch->GetIntPref(NS_PREF_FOLDERLIST,
                              &val);
  NS_ENSURE_SUCCESS(rv, rv);

  switch(val) {
    case 0: 
      {
        nsCOMPtr<nsILocalFile> downloadDir;
        nsCOMPtr<nsIProperties> dirService =
           do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = dirService->Get(NS_OS_DESKTOP_DIR,
                             NS_GET_IID(nsILocalFile),
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
        nsCOMPtr<nsILocalFile> customDirectory;
        prefBranch->GetComplexValue(NS_PREF_DIR,
                                    NS_GET_IID(nsILocalFile),
                                    getter_AddRefs(customDirectory));
        if (customDirectory) {
          PRBool exists = PR_FALSE;
          (void)customDirectory->Exists(&exists);

          if (!exists) {
            rv = customDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
            if (NS_SUCCEEDED(rv)) {
              customDirectory.forget(aResult);
              return NS_OK;
            }

            
            
          }

          PRBool writable = PR_FALSE;
          PRBool directory = PR_FALSE;
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
                                            NS_GET_IID(nsILocalFile),
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
                               nsILocalFile *aTempFile,
                               nsICancelable *aCancelable,
                               nsIDownload **aDownload)
{
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

  dl->mDisplayName = aDisplayName;
  if (dl->mDisplayName.IsEmpty())
    targetFile->GetLeafName(dl->mDisplayName);

  dl->mMIMEInfo = aMIMEInfo;
  dl->SetStartTime(aStartTime == 0 ? PR_Now() : aStartTime);

  
  dl->mCancelable = aCancelable;

  
  nsCAutoString source, target;
  aSource->GetSpec(source);
  aTarget->GetSpec(target);

  
  nsAutoString tempPath;
  if (aTempFile)
    aTempFile->GetPath(tempPath);

  
  
  nsCAutoString persistentDescriptor, mimeType;
  nsHandlerInfoAction action = nsIMIMEInfo::saveToDisk;
  if (aMIMEInfo) {
    (void)aMIMEInfo->GetType(mimeType);

    nsCOMPtr<nsIHandlerApp> handlerApp;
    (void)aMIMEInfo->GetPreferredApplicationHandler(getter_AddRefs(handlerApp));
    nsCOMPtr<nsILocalHandlerApp> locHandlerApp = do_QueryInterface(handlerApp);

    if (locHandlerApp) {
      nsCOMPtr<nsIFile> executable;
      (void)locHandlerApp->GetExecutable(getter_AddRefs(executable));
      nsCOMPtr<nsILocalFile> locExecutable = do_QueryInterface(executable);

      if (locExecutable)
        (void)locExecutable->GetPersistentDescriptor(persistentDescriptor);
    }

    (void)aMIMEInfo->GetPreferredAction(&action);
  }

  DownloadState startState = nsIDownloadManager::DOWNLOAD_QUEUED;

  PRInt64 id = AddDownloadToDB(dl->mDisplayName, source, target, tempPath,
                               dl->mStartTime, dl->mLastUpdate,
                               mimeType, persistentDescriptor, action);
  NS_ENSURE_TRUE(id, NS_ERROR_FAILURE);
  dl->mID = id;

  rv = AddToCurrentDownloads(dl);
  (void)dl->SetState(startState);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DOWNLOAD_SCANNER
  if (mScanner) {
    AVCheckPolicyState res = mScanner->CheckPolicy(aSource, aTarget);
    if (res == AVPOLICY_BLOCKED) {
      
      
      (void)CancelDownload(id);
      startState = nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY;
    }
  }
#endif

  
  
  
  nsCOMPtr<nsIParentalControlsService> pc =
    do_CreateInstance(NS_PARENTALCONTROLSSERVICE_CONTRACTID);
  if (pc) {
    PRBool enabled = PR_FALSE;
    (void)pc->GetBlockFileDownloadsEnabled(&enabled);
    if (enabled) {
      (void)CancelDownload(id);
      (void)dl->SetState(nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL);
    }

    
    PRBool logEnabled = PR_FALSE;
    (void)pc->GetLoggingEnabled(&logEnabled);
    if (logEnabled) {
      (void)pc->Log(nsIParentalControlsService::ePCLog_FileDownload,
                    enabled,
                    aSource,
                    nsnull);
    }
  }

  NS_ADDREF(*aDownload = dl);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetDownload(PRUint32 aID, nsIDownload **aDownloadItem)
{
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

nsDownload *
nsDownloadManager::FindDownload(PRUint32 aID)
{
  
  for (PRInt32 i = mCurrentDownloads.Count() - 1; i >= 0; --i) {
    nsDownload *dl = mCurrentDownloads[i];

    if (dl->mID == aID)
      return dl;
  }

  return nsnull;
}

NS_IMETHODIMP
nsDownloadManager::CancelDownload(PRUint32 aID)
{
  
  nsRefPtr<nsDownload> dl = FindDownload(aID);

  
  if (!dl)
    return NS_ERROR_FAILURE;

  
  if (dl->IsFinished())
    return NS_OK;

  
  if (dl->IsPaused() && !dl->IsResumable())
    (void)dl->Resume();

  
  (void)dl->Cancel();

  
  
  
  
  if (dl->mTempFile) {
    PRBool exists;
    dl->mTempFile->Exists(&exists);
    if (exists)
      dl->mTempFile->Remove(PR_FALSE);
  }

  nsresult rv = dl->SetState(nsIDownloadManager::DOWNLOAD_CANCELED);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::RetryDownload(PRUint32 aID)
{
  nsRefPtr<nsDownload> dl;
  nsresult rv = GetDownloadFromDB(aID, getter_AddRefs(dl));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (dl->mDownloadState != nsIDownloadManager::DOWNLOAD_FAILED &&
      dl->mDownloadState != nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL &&
      dl->mDownloadState != nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY &&
      dl->mDownloadState != nsIDownloadManager::DOWNLOAD_DIRTY &&
      dl->mDownloadState != nsIDownloadManager::DOWNLOAD_CANCELED)
    return NS_ERROR_FAILURE;

  
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

  rv = wbp->SaveURI(dl->mSource, nsnull, nsnull, nsnull, nsnull, dl->mTarget);
  if (NS_FAILED(rv)) {
    dl->mCancelable = nsnull;
    (void)wbp->SetProgressListener(nsnull);
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::RemoveDownload(PRUint32 aID)
{
  nsDownload *dl = FindDownload(aID);
  NS_ASSERTION(!dl, "Can't call RemoveDownload on a download in progress!");
  if (dl)
    return NS_ERROR_FAILURE;

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_downloads "
    "WHERE id = ?1"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt64Parameter(0, aID); 
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsPRUint32> id =
    do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = id->SetData(aID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  return mObserverService->NotifyObservers(id,
                                           "download-manager-remove-download",
                                           nsnull);
}

NS_IMETHODIMP
nsDownloadManager::RemoveDownloadsByTimeframe(PRInt64 aStartTime,
                                              PRInt64 aEndTime)
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_downloads "
    "WHERE startTime >= ?1 "
    "AND startTime <= ?2 "
    "AND state NOT IN (?3, ?4, ?5)"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt64Parameter(0, aStartTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt64Parameter(1, aEndTime);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt32Parameter(2, nsIDownloadManager::DOWNLOAD_DOWNLOADING);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(3, nsIDownloadManager::DOWNLOAD_PAUSED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32Parameter(4, nsIDownloadManager::DOWNLOAD_QUEUED);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  return mObserverService->NotifyObservers(nsnull,
                                           "download-manager-remove-download",
                                           nsnull);
}

NS_IMETHODIMP
nsDownloadManager::CleanUp()
{
  DownloadState states[] = { nsIDownloadManager::DOWNLOAD_FINISHED,
                             nsIDownloadManager::DOWNLOAD_FAILED,
                             nsIDownloadManager::DOWNLOAD_CANCELED,
                             nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL,
                             nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY,
                             nsIDownloadManager::DOWNLOAD_DIRTY };

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_downloads "
    "WHERE state = ?1 "
      "OR state = ?2 "
      "OR state = ?3 "
      "OR state = ?4 "
      "OR state = ?5 "
      "OR state = ?6"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(states); ++i) {
    rv = stmt->BindInt32Parameter(i, states[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  return mObserverService->NotifyObservers(nsnull,
                                           "download-manager-remove-download",
                                           nsnull);
}

NS_IMETHODIMP
nsDownloadManager::GetCanCleanUp(PRBool *aResult)
{
  *aResult = PR_FALSE;

  DownloadState states[] = { nsIDownloadManager::DOWNLOAD_FINISHED,
                             nsIDownloadManager::DOWNLOAD_FAILED,
                             nsIDownloadManager::DOWNLOAD_CANCELED,
                             nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL,
                             nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY,
                             nsIDownloadManager::DOWNLOAD_DIRTY };

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT COUNT(*) "
    "FROM moz_downloads "
    "WHERE state = ?1 "
      "OR state = ?2 "
      "OR state = ?3 "
      "OR state = ?4 "
      "OR state = ?5 "
      "OR state = ?6"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(states); ++i) {
    rv = stmt->BindInt32Parameter(i, states[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool moreResults; 
  rv = stmt->ExecuteStep(&moreResults);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 count;
  rv = stmt->GetInt32(0, &count);

  if (count > 0)
    *aResult = PR_TRUE;

  return rv;
}

NS_IMETHODIMP
nsDownloadManager::PauseDownload(PRUint32 aID)
{
  nsDownload *dl = FindDownload(aID);
  if (!dl)
    return NS_ERROR_FAILURE;

  return dl->Pause();
}

NS_IMETHODIMP
nsDownloadManager::ResumeDownload(PRUint32 aID)
{
  nsDownload *dl = FindDownload(aID);
  if (!dl)
    return NS_ERROR_FAILURE;

  return dl->Resume();
}

NS_IMETHODIMP
nsDownloadManager::GetDBConnection(mozIStorageConnection **aDBConn)
{
  NS_ADDREF(*aDBConn = mDBConn);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::AddListener(nsIDownloadProgressListener *aListener)
{
  mListeners.AppendObject(aListener);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::RemoveListener(nsIDownloadProgressListener *aListener)
{
  mListeners.RemoveObject(aListener);

  return NS_OK;
}

void
nsDownloadManager::NotifyListenersOnDownloadStateChange(PRInt16 aOldState,
                                                        nsIDownload *aDownload)
{
  for (PRInt32 i = mListeners.Count() - 1; i >= 0; --i)
    mListeners[i]->OnDownloadStateChange(aOldState, aDownload);
}

void
nsDownloadManager::NotifyListenersOnProgressChange(nsIWebProgress *aProgress,
                                                   nsIRequest *aRequest,
                                                   PRInt64 aCurSelfProgress,
                                                   PRInt64 aMaxSelfProgress,
                                                   PRInt64 aCurTotalProgress,
                                                   PRInt64 aMaxTotalProgress,
                                                   nsIDownload *aDownload)
{
  for (PRInt32 i = mListeners.Count() - 1; i >= 0; --i)
    mListeners[i]->OnProgressChange(aProgress, aRequest, aCurSelfProgress,
                                    aMaxSelfProgress, aCurTotalProgress,
                                    aMaxTotalProgress, aDownload);
}

void
nsDownloadManager::NotifyListenersOnStateChange(nsIWebProgress *aProgress,
                                                nsIRequest *aRequest,
                                                PRUint32 aStateFlags,
                                                nsresult aStatus,
                                                nsIDownload *aDownload)
{
  for (PRInt32 i = mListeners.Count() - 1; i >= 0; --i)
    mListeners[i]->OnStateChange(aProgress, aRequest, aStateFlags, aStatus,
                                 aDownload);
}

nsresult
nsDownloadManager::SwitchDatabaseTypeTo(enum nsDownloadManager::DatabaseType aType)
{
  if (aType == mDBType)
    return NS_OK; 

  mDBType = aType;

  (void)PauseAllDownloads(PR_TRUE);
  (void)RemoveAllDownloads();

  nsresult rv = InitDB();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = RestoreDatabaseState();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RestoreActiveDownloads();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to restore all active downloads");

  return rv;
}




NS_IMETHODIMP
nsDownloadManager::OnBeginUpdateBatch()
{
  
  if (mHistoryTransaction)
    return NS_OK;

  
  mHistoryTransaction = new mozStorageTransaction(mDBConn, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnEndUpdateBatch()
{
  
  mHistoryTransaction = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnVisit(nsIURI *aURI, PRInt64 aVisitID, PRTime aTime,
                           PRInt64 aSessionID, PRInt64 aReferringID,
                           PRUint32 aTransitionType, PRUint32 *aAdded)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnTitleChanged(nsIURI *aURI, const nsAString &aPageTitle)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnBeforeDeleteURI(nsIURI *aURI)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnDeleteURI(nsIURI *aURI)
{
  return RemoveDownloadsForURI(aURI);
}

NS_IMETHODIMP
nsDownloadManager::OnClearHistory()
{
  return CleanUp();
}

NS_IMETHODIMP
nsDownloadManager::OnPageChanged(nsIURI *aURI, PRUint32 aWhat,
                                 const nsAString &aValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::OnPageExpired(nsIURI *aURI, PRTime aVisitTime,
                                 PRBool aWholeEntry)
{
  
  if (!aWholeEntry)
    return NS_OK;

  return RemoveDownloadsForURI(aURI);
}




NS_IMETHODIMP
nsDownloadManager::Observe(nsISupports *aSubject,
                           const char *aTopic,
                           const PRUnichar *aData)
{
  PRInt32 currDownloadCount = mCurrentDownloads.Count();

  
  
  if (GetQuitBehavior() != QUIT_AND_CANCEL)
    for (PRInt32 i = currDownloadCount - 1; i >= 0; --i)
      if (mCurrentDownloads[i]->IsResumable())
        currDownloadCount--;

  nsresult rv;
  if (strcmp(aTopic, "oncancel") == 0) {
    nsCOMPtr<nsIDownload> dl = do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 id;
    dl->GetId(&id);
    nsDownload *dl2 = FindDownload(id);
    if (dl2)
      return CancelDownload(id);
  } else if (strcmp(aTopic, "quit-application") == 0) {
    
    
    enum QuitBehavior behavior = GetQuitBehavior();
    if (behavior != QUIT_AND_CANCEL)
      (void)PauseAllDownloads(PRBool(behavior != QUIT_AND_PAUSE));

    
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
                           NS_LITERAL_STRING("quitCancelDownloadsAlertTitle").get(),
                           NS_LITERAL_STRING("quitCancelDownloadsAlertMsgMultiple").get(),
                           NS_LITERAL_STRING("quitCancelDownloadsAlertMsg").get(),
                           NS_LITERAL_STRING("dontQuitButtonWin").get());
#else
    ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                           NS_LITERAL_STRING("quitCancelDownloadsAlertTitle").get(),
                           NS_LITERAL_STRING("quitCancelDownloadsAlertMsgMacMultiple").get(),
                           NS_LITERAL_STRING("quitCancelDownloadsAlertMsgMac").get(),
                           NS_LITERAL_STRING("dontQuitButtonMac").get());
#endif
  } else if (strcmp(aTopic, "offline-requested") == 0 && currDownloadCount) {
    nsCOMPtr<nsISupportsPRBool> cancelDownloads =
      do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                           NS_LITERAL_STRING("offlineCancelDownloadsAlertTitle").get(),
                           NS_LITERAL_STRING("offlineCancelDownloadsAlertMsgMultiple").get(),
                           NS_LITERAL_STRING("offlineCancelDownloadsAlertMsg").get(),
                           NS_LITERAL_STRING("dontGoOfflineButton").get());
  }
  else if (strcmp(aTopic, NS_IOSERVICE_GOING_OFFLINE_TOPIC) == 0) {
    
    (void)PauseAllDownloads(PR_TRUE);
  }
  else if (strcmp(aTopic, NS_IOSERVICE_OFFLINE_STATUS_TOPIC) == 0 &&
           nsDependentString(aData).EqualsLiteral(NS_IOSERVICE_ONLINE)) {
    
    (void)ResumeAllDownloads(PR_FALSE);
  }
  else if (strcmp(aTopic, "dlmgr-switchdb") == 0) {
    if (NS_LITERAL_STRING("memory").Equals(aData))
      return SwitchDatabaseTypeTo(DATABASE_MEMORY);
    else if (NS_LITERAL_STRING("disk").Equals(aData))
      return SwitchDatabaseTypeTo(DATABASE_DISK);
  }
  else if (strcmp(aTopic, "alertclickcallback") == 0) {
    nsCOMPtr<nsIDownloadManagerUI> dmui =
      do_GetService("@mozilla.org/download-manager-ui;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    return dmui->Show(nsnull, 0, nsIDownloadManagerUI::REASON_USER_INTERACTED);
  } else if (strcmp(aTopic, "sleep_notification") == 0) {
    
    (void)PauseAllDownloads(PR_TRUE);
  } else if (strcmp(aTopic, "wake_notification") == 0) {
    PRInt32 resumeOnWakeDelay = 10000;
    nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (pref)
      (void)pref->GetIntPref(PREF_BDM_RESUMEONWAKEDELAY, &resumeOnWakeDelay);

    
    
    mResumeOnWakeTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (resumeOnWakeDelay >= 0 && mResumeOnWakeTimer) {
      (void)mResumeOnWakeTimer->InitWithFuncCallback(ResumeOnWakeCallback,
        this, resumeOnWakeDelay, nsITimer::TYPE_ONE_SHOT);
    }
  }
  else if (strcmp(aTopic, NS_PRIVATE_BROWSING_REQUEST_TOPIC) == 0 &&
           currDownloadCount) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(aData)) {
      nsCOMPtr<nsISupportsPRBool> cancelDownloads =
        do_QueryInterface(aSubject, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                             NS_LITERAL_STRING("enterPrivateBrowsingCancelDownloadsAlertTitle").get(),
                             NS_LITERAL_STRING("enterPrivateBrowsingCancelDownloadsAlertMsgMultiple").get(),
                             NS_LITERAL_STRING("enterPrivateBrowsingCancelDownloadsAlertMsg").get(),
                             NS_LITERAL_STRING("dontEnterPrivateBrowsingButton").get());
    }
    else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData)) {
      nsCOMPtr<nsISupportsPRBool> cancelDownloads =
        do_QueryInterface(aSubject, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                             NS_LITERAL_STRING("leavePrivateBrowsingCancelDownloadsAlertTitle").get(),
                             NS_LITERAL_STRING("leavePrivateBrowsingCancelDownloadsAlertMsgMultiple").get(),
                             NS_LITERAL_STRING("leavePrivateBrowsingCancelDownloadsAlertMsg").get(),
                             NS_LITERAL_STRING("dontLeavePrivateBrowsingButton").get());
    }
  }
  else if (strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC) == 0) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(aData))
      OnEnterPrivateBrowsingMode();
    else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData))
      OnLeavePrivateBrowsingMode();
  }

  return NS_OK;
}

void
nsDownloadManager::OnEnterPrivateBrowsingMode()
{
  
  (void)PauseAllDownloads(PR_TRUE);

  
  (void)SwitchDatabaseTypeTo(DATABASE_MEMORY);

  mInPrivateBrowsing = PR_TRUE;
}

void
nsDownloadManager::OnLeavePrivateBrowsingMode()
{
  
  (void)ResumeAllDownloads(PR_FALSE);

  
  (void)SwitchDatabaseTypeTo(DATABASE_DISK);

  mInPrivateBrowsing = PR_FALSE;
}

void
nsDownloadManager::ConfirmCancelDownloads(PRInt32 aCount,
                                          nsISupportsPRBool *aCancelDownloads,
                                          const PRUnichar *aTitle,
                                          const PRUnichar *aCancelMessageMultiple,
                                          const PRUnichar *aCancelMessageSingle,
                                          const PRUnichar *aDontCancelButton)
{
  
  PRBool quitRequestCancelled = PR_FALSE;
  aCancelDownloads->GetData(&quitRequestCancelled);
  if (quitRequestCancelled)
    return;

  nsXPIDLString title, message, quitButton, dontQuitButton;

  mBundle->GetStringFromName(aTitle, getter_Copies(title));

  nsAutoString countString;
  countString.AppendInt(aCount);
  const PRUnichar *strings[1] = { countString.get() };
  if (aCount > 1) {
    mBundle->FormatStringFromName(aCancelMessageMultiple, strings, 1,
                                  getter_Copies(message));
    mBundle->FormatStringFromName(NS_LITERAL_STRING("cancelDownloadsOKTextMultiple").get(),
                                  strings, 1, getter_Copies(quitButton));
  } else {
    mBundle->GetStringFromName(aCancelMessageSingle, getter_Copies(message));
    mBundle->GetStringFromName(NS_LITERAL_STRING("cancelDownloadsOKText").get(),
                               getter_Copies(quitButton));
  }

  mBundle->GetStringFromName(aDontCancelButton, getter_Copies(dontQuitButton));

  
  nsCOMPtr<nsIWindowMediator> wm = do_GetService(NS_WINDOWMEDIATOR_CONTRACTID);
  nsCOMPtr<nsIDOMWindowInternal> dmWindow;
  if (wm) {
    wm->GetMostRecentWindow(NS_LITERAL_STRING("Download:Manager").get(),
                            getter_AddRefs(dmWindow));
  }

  
  nsCOMPtr<nsIPromptService> prompter(do_GetService(NS_PROMPTSERVICE_CONTRACTID));
  if (prompter) {
    PRInt32 flags = (nsIPromptService::BUTTON_TITLE_IS_STRING * nsIPromptService::BUTTON_POS_0) + (nsIPromptService::BUTTON_TITLE_IS_STRING * nsIPromptService::BUTTON_POS_1);
    PRBool nothing = PR_FALSE;
    PRInt32 button;
    prompter->ConfirmEx(dmWindow, title, message, flags, quitButton.get(), dontQuitButton.get(), nsnull, nsnull, &nothing, &button);

    aCancelDownloads->SetData(button == 1);
  }
}




NS_IMPL_ISUPPORTS4(nsDownload, nsIDownload, nsITransfer, nsIWebProgressListener,
                   nsIWebProgressListener2)

nsDownload::nsDownload() : mDownloadState(nsIDownloadManager::DOWNLOAD_NOTSTARTED),
                           mID(0),
                           mPercentComplete(0),
                           mCurrBytes(0),
                           mMaxBytes(-1),
                           mStartTime(0),
                           mLastUpdate(PR_Now() - (PRUint32)gUpdateInterval),
                           mResumedAt(-1),
                           mSpeed(0),
                           mHasMultipleFiles(PR_FALSE),
                           mAutoResume(DONT_RESUME)
{
}

nsDownload::~nsDownload()
{
}

nsresult
nsDownload::SetState(DownloadState aState)
{
  NS_ASSERTION(mDownloadState != aState,
               "Trying to set the download state to what it already is set to!");

  PRInt16 oldState = mDownloadState;
  mDownloadState = aState;

  nsresult rv;

  nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);

  
  nsRefPtr<nsDownload> kungFuDeathGrip = this;

  
  
  
  
  
  
  switch (aState) {
    case nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL:
    case nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY:
    case nsIDownloadManager::DOWNLOAD_DIRTY:
    case nsIDownloadManager::DOWNLOAD_CANCELED:
    case nsIDownloadManager::DOWNLOAD_FAILED:
      
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
      
      (void)ExecuteDesiredAction();

      
      Finalize();

      
      PRBool showTaskbarAlert = PR_TRUE;
      if (pref)
        pref->GetBoolPref(PREF_BDM_SHOWALERTONCOMPLETE, &showTaskbarAlert);

      if (showTaskbarAlert) {
        PRInt32 alertInterval = 2000;
        if (pref)
          pref->GetIntPref(PREF_BDM_SHOWALERTINTERVAL, &alertInterval);

        PRInt64 alertIntervalUSec = alertInterval * PR_USEC_PER_MSEC;
        PRInt64 goat = PR_Now() - mStartTime;
        showTaskbarAlert = goat > alertIntervalUSec;

        PRInt32 size = mDownloadManager->mCurrentDownloads.Count();
        if (showTaskbarAlert && size == 0) {
          nsCOMPtr<nsIAlertsService> alerts =
            do_GetService("@mozilla.org/alerts-service;1");
          if (alerts) {
              nsXPIDLString title, message;

              mDownloadManager->mBundle->GetStringFromName(
                  NS_LITERAL_STRING("downloadsCompleteTitle").get(),
                  getter_Copies(title));
              mDownloadManager->mBundle->GetStringFromName(
                  NS_LITERAL_STRING("downloadsCompleteMsg").get(),
                  getter_Copies(message));

              PRBool removeWhenDone =
                mDownloadManager->GetRetentionBehavior() == 0;

              
              
              
              
              alerts->ShowAlertNotification(
                  NS_LITERAL_STRING(DOWNLOAD_MANAGER_ALERT_ICON), title,
                  message, !removeWhenDone, EmptyString(), mDownloadManager,
                  EmptyString());
            }
        }
      }
#if defined(XP_WIN) && !defined(WINCE)
      nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(mTarget);
      nsCOMPtr<nsIFile> file;
      nsAutoString path;

      if (fileURL &&
          NS_SUCCEEDED(fileURL->GetFile(getter_AddRefs(file))) &&
          file &&
          NS_SUCCEEDED(file->GetPath(path))) {

        
        
        {
          PRBool addToRecentDocs = PR_TRUE;
          if (pref)
            pref->GetBoolPref(PREF_BDM_ADDTORECENTDOCS, &addToRecentDocs);

          if (addToRecentDocs)
            ::SHAddToRecentDocs(SHARD_PATHW, path.get());
        }
      }

      
      
      
      nsCOMPtr<nsIFile> tempDir, fileDir;
      rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tempDir));
      NS_ENSURE_SUCCESS(rv, rv);
      (void)file->GetParent(getter_AddRefs(fileDir));

      PRBool isTemp = PR_FALSE;
      if (fileDir)
        (void)fileDir->Equals(tempDir, &isTemp);

      nsCOMPtr<nsILocalFileWin> localFileWin(do_QueryInterface(file));
      if (!isTemp && localFileWin)
        (void)localFileWin->SetFileAttributesWin(nsILocalFileWin::WFA_SEARCH_INDEXED);

#endif
      
      if (mDownloadManager->GetRetentionBehavior() == 0)
        mDownloadManager->RemoveDownload(mID);
    }
    break;
  default:
    break;
  }

  
  
  rv = UpdateDB();
  NS_ENSURE_SUCCESS(rv, rv);

  mDownloadManager->NotifyListenersOnDownloadStateChange(oldState, this);

  switch (mDownloadState) {
    case nsIDownloadManager::DOWNLOAD_DOWNLOADING:
      
      if (oldState == nsIDownloadManager::DOWNLOAD_QUEUED)
        mDownloadManager->SendEvent(this, "dl-start");
      break;
    case nsIDownloadManager::DOWNLOAD_FAILED:
      mDownloadManager->SendEvent(this, "dl-failed");
      break;
    case nsIDownloadManager::DOWNLOAD_SCANNING:
      mDownloadManager->SendEvent(this, "dl-scanning");
      break;
    case nsIDownloadManager::DOWNLOAD_FINISHED:
      mDownloadManager->SendEvent(this, "dl-done");
      break;
    case nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL:
    case nsIDownloadManager::DOWNLOAD_BLOCKED_POLICY:
      mDownloadManager->SendEvent(this, "dl-blocked");
      break;
    case nsIDownloadManager::DOWNLOAD_DIRTY:
      mDownloadManager->SendEvent(this, "dl-dirty");
      break;
    case nsIDownloadManager::DOWNLOAD_CANCELED:
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
                               PRInt64 aCurSelfProgress,
                               PRInt64 aMaxSelfProgress,
                               PRInt64 aCurTotalProgress,
                               PRInt64 aMaxTotalProgress)
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

    
    
    if (!mMIMEInfo) {
      nsCOMPtr<nsIDownloadHistory> dh =
        do_GetService(NS_DOWNLOADHISTORY_CONTRACTID);
      if (dh)
        (void)dh->AddDownload(mSource, mReferrer, mStartTime);
    }

    
    nsCOMPtr<nsIResumableChannel> resumableChannel(do_QueryInterface(aRequest));
    if (resumableChannel)
      (void)resumableChannel->GetEntityID(mEntityID);

    
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

  
  PRInt64 currBytes, maxBytes;
  (void)GetAmountTransferred(&currBytes);
  (void)GetSize(&maxBytes);
  mDownloadManager->NotifyListenersOnProgressChange(
    aWebProgress, aRequest, currBytes, maxBytes, currBytes, maxBytes, this);

  
  if (aMaxSelfProgress != aMaxTotalProgress)
    mHasMultipleFiles = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnRefreshAttempted(nsIWebProgress *aWebProgress,
                               nsIURI *aUri,
                               PRInt32 aDelay,
                               PRBool aSameUri,
                               PRBool *allowRefresh)
{
  *allowRefresh = PR_TRUE;
  return NS_OK;
}




NS_IMETHODIMP
nsDownload::OnProgressChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest,
                             PRInt32 aCurSelfProgress,
                             PRInt32 aMaxSelfProgress,
                             PRInt32 aCurTotalProgress,
                             PRInt32 aMaxTotalProgress)
{
  return OnProgressChange64(aWebProgress, aRequest,
                            aCurSelfProgress, aMaxSelfProgress,
                            aCurTotalProgress, aMaxTotalProgress);
}

NS_IMETHODIMP
nsDownload::OnLocationChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest, nsIURI *aLocation)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnStatusChange(nsIWebProgress *aWebProgress,
                           nsIRequest *aRequest, nsresult aStatus,
                           const PRUnichar *aMessage)
{
  if (NS_FAILED(aStatus))
    return FailDownload(aStatus, aMessage);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnStateChange(nsIWebProgress *aWebProgress,
                          nsIRequest *aRequest, PRUint32 aStateFlags,
                          nsresult aStatus)
{
  
  nsRefPtr<nsDownload> kungFuDeathGrip = this;

  
  
  if ((aStateFlags & STATE_START) && (aStateFlags & STATE_IS_NETWORK)) {
    nsresult rv;
    nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(aRequest, &rv);
    if (NS_SUCCEEDED(rv)) {
      PRUint32 status;
      rv = channel->GetResponseStatus(&status);
      
      if (NS_SUCCEEDED(rv) && status == 450) {
        
        (void)Cancel();

        
        (void)SetState(nsIDownloadManager::DOWNLOAD_BLOCKED_PARENTAL);
      }
    }
  } else if ((aStateFlags & STATE_STOP) && (aStateFlags & STATE_IS_NETWORK) &&
             IsFinishable()) {
    
    
    if (NS_SUCCEEDED(aStatus)) {
      
      
      
      
      PRInt64 fileSize;
      nsCOMPtr<nsILocalFile> file;
      
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
      PRBool scan = PR_TRUE;
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
      
      (void)FailDownload(aStatus, nsnull);
    }
  }

  mDownloadManager->NotifyListenersOnStateChange(aWebProgress, aRequest,
                                                 aStateFlags, aStatus, this);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnSecurityChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest, PRUint32 aState)
{
  return NS_OK;
}




NS_IMETHODIMP
nsDownload::Init(nsIURI *aSource,
                 nsIURI *aTarget,
                 const nsAString& aDisplayName,
                 nsIMIMEInfo *aMIMEInfo,
                 PRTime aStartTime,
                 nsILocalFile *aTempFile,
                 nsICancelable *aCancelable)
{
  NS_WARNING("Huh...how did we get here?!");
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetState(PRInt16 *aState)
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
nsDownload::GetStartTime(PRInt64 *aStartTime)
{
  *aStartTime = mStartTime;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetPercentComplete(PRInt32 *aPercentComplete)
{
  *aPercentComplete = mPercentComplete;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetAmountTransferred(PRInt64 *aAmountTransferred)
{
  *aAmountTransferred = mCurrBytes + (WasResumed() ? mResumedAt : 0);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetSize(PRInt64 *aSize)
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
nsDownload::GetTargetFile(nsILocalFile **aTargetFile)
{
  nsresult rv;

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(mTarget, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  if (NS_SUCCEEDED(rv))
    rv = CallQueryInterface(file, aTargetFile);
  return rv;
}

NS_IMETHODIMP
nsDownload::GetSpeed(double *aSpeed)
{
  *aSpeed = mSpeed;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetId(PRUint32 *aId)
{
  *aId = mID;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetReferrer(nsIURI **referrer)
{
  NS_IF_ADDREF(*referrer = mReferrer);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetResumable(PRBool *resumable)
{
  *resumable = IsResumable();
  return NS_OK;
}




void
nsDownload::Finalize()
{
  
  mCancelable = nsnull;

  
  mEntityID.Truncate();
  mTempFile = nsnull;

  
  (void)mDownloadManager->mCurrentDownloads.RemoveObject(this);

  
  mAutoResume = DONT_RESUME;
}

nsresult
nsDownload::ExecuteDesiredAction()
{
  
  
  if (!mTempFile || !WasResumed())
    return NS_OK;

  
  PRBool fileExists;
  if (NS_FAILED(mTempFile->Exists(&fileExists)) || !fileExists)
    return NS_ERROR_FILE_NOT_FOUND;

  
  nsHandlerInfoAction action = nsIMIMEInfo::saveToDisk;
  if (mMIMEInfo) {
    nsresult rv = mMIMEInfo->GetPreferredAction(&action);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsresult retVal = NS_OK;
  switch (action) {
    case nsIMIMEInfo::saveToDisk:
      
      retVal = MoveTempToTarget();
      break;
    case nsIMIMEInfo::useHelperApp:
    case nsIMIMEInfo::useSystemDefault:
      
      
      retVal = OpenWithApplication();
      break;
    default:
      break;
  }

  return retVal;
}

nsresult
nsDownload::MoveTempToTarget()
{
  nsCOMPtr<nsILocalFile> target;
  nsresult rv = GetTargetFile(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  PRBool fileExists;
  if (NS_SUCCEEDED(target->Exists(&fileExists)) && fileExists) {
    rv = target->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsAutoString fileName;
  rv = target->GetLeafName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFile> dir;
  rv = target->GetParent(getter_AddRefs(dir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTempFile->MoveTo(dir, fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDownload::OpenWithApplication()
{
  
  nsCOMPtr<nsILocalFile> target;
  nsresult rv = GetTargetFile(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = target->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = MoveTempToTarget();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  nsresult retVal = mMIMEInfo->LaunchWithFile(target);

  PRBool deleteTempFileOnExit;
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefs || NS_FAILED(prefs->GetBoolPref(PREF_BH_DELETETEMPFILEONEXIT,
                                             &deleteTempFileOnExit))) {
    
#if !defined(XP_MACOSX)
    
    
    
    deleteTempFileOnExit = PR_TRUE;
#else
    deleteTempFileOnExit = PR_FALSE;
#endif
  }

  
  
  if (deleteTempFileOnExit ||
      nsDownloadManager::gDownloadManagerService->mInPrivateBrowsing) {
    
    
    nsCOMPtr<nsPIExternalAppLauncher> appLauncher(do_GetService
                    (NS_EXTERNALHELPERAPPSERVICE_CONTRACTID));

    
    
    if (appLauncher)
      (void)appLauncher->DeleteTemporaryFileOnExit(target);
  }

  return retVal;
}

void
nsDownload::SetStartTime(PRInt64 aStartTime)
{
  mStartTime = aStartTime;
  mLastUpdate = aStartTime;
}

void
nsDownload::SetProgressBytes(PRInt64 aCurrBytes, PRInt64 aMaxBytes)
{
  mCurrBytes = aCurrBytes;
  mMaxBytes = aMaxBytes;

  
  PRInt64 currBytes, maxBytes;
  (void)GetAmountTransferred(&currBytes);
  (void)GetSize(&maxBytes);

  if (currBytes == maxBytes)
    mPercentComplete = 100;
  else if (maxBytes <= 0)
    mPercentComplete = -1;
  else
    mPercentComplete = (PRInt32)((PRFloat64)currBytes / maxBytes * 100 + .5);
}

nsresult
nsDownload::Pause()
{
  if (!IsResumable())
    return NS_ERROR_UNEXPECTED;

  nsresult rv = Cancel();
  NS_ENSURE_SUCCESS(rv, rv);

  return SetState(nsIDownloadManager::DOWNLOAD_PAUSED);
}

nsresult
nsDownload::Cancel()
{
  nsresult rv = NS_OK;
  if (mCancelable) {
    rv = mCancelable->Cancel(NS_BINDING_ABORTED);
    
    mCancelable = nsnull;
  }

  return rv;
}

nsresult
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
  rv = NS_NewChannel(getter_AddRefs(channel), mSource, nsnull, nsnull, ir);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsILocalFile> targetLocalFile(mTempFile);
  if (!targetLocalFile) {
    rv = GetTargetFile(getter_AddRefs(targetLocalFile));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  PRInt64 fileSize;
  
  nsCOMPtr<nsIFile> clone;
  if (NS_FAILED(targetLocalFile->Clone(getter_AddRefs(clone))) ||
      NS_FAILED(clone->GetFileSize(&fileSize)))
    fileSize = 0;

  
  nsCOMPtr<nsIResumableChannel> resumableChannel(do_QueryInterface(channel));
  if (!resumableChannel)
    return NS_ERROR_UNEXPECTED;
  rv = resumableChannel->ResumeAt(fileSize, mEntityID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 maxBytes;
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
    mCancelable = nsnull;
    (void)wbp->SetProgressListener(nsnull);
    return rv;
  }

  return SetState(nsIDownloadManager::DOWNLOAD_DOWNLOADING);
}

PRBool
nsDownload::IsPaused()
{
  return mDownloadState == nsIDownloadManager::DOWNLOAD_PAUSED;
}

PRBool
nsDownload::IsResumable()
{
  return !mEntityID.IsEmpty();
}

PRBool
nsDownload::WasResumed()
{
  return mResumedAt != -1;
}

PRBool
nsDownload::ShouldAutoResume()
{
  return mAutoResume == AUTO_RESUME;
}

PRBool
nsDownload::IsFinishable()
{
  return mDownloadState == nsIDownloadManager::DOWNLOAD_NOTSTARTED ||
         mDownloadState == nsIDownloadManager::DOWNLOAD_QUEUED ||
         mDownloadState == nsIDownloadManager::DOWNLOAD_DOWNLOADING;
}

PRBool
nsDownload::IsFinished()
{
  return mDownloadState == nsIDownloadManager::DOWNLOAD_FINISHED;
}

nsresult
nsDownload::UpdateDB()
{
  NS_ASSERTION(mID, "Download ID is stored as zero.  This is bad!");
  NS_ASSERTION(mDownloadManager, "Egads!  We have no download manager!");

  mozIStorageStatement *stmt = mDownloadManager->mUpdateDownloadStatement;

  PRInt32 i = 0;
  
  nsAutoString tempPath;
  if (mTempFile)
    (void)mTempFile->GetPath(tempPath);
  nsresult rv = stmt->BindStringParameter(i++, tempPath);

  
  rv = stmt->BindInt64Parameter(i++, mStartTime);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt64Parameter(i++, mLastUpdate);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt32Parameter(i++, mDownloadState);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mReferrer) {
    nsCAutoString referrer;
    rv = mReferrer->GetSpec(referrer);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindUTF8StringParameter(i++, referrer);
  } else {
    rv = stmt->BindNullParameter(i++);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindUTF8StringParameter(i++, mEntityID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 currBytes;
  (void)GetAmountTransferred(&currBytes);
  rv = stmt->BindInt64Parameter(i++, currBytes);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 maxBytes;
  (void)GetSize(&maxBytes);
  rv = stmt->BindInt64Parameter(i++, maxBytes);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt32Parameter(i++, mAutoResume);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt64Parameter(i++, mID);
  NS_ENSURE_SUCCESS(rv, rv);

  return stmt->Execute();
}

nsresult
nsDownload::FailDownload(nsresult aStatus, const PRUnichar *aMessage)
{
  
  nsCOMPtr<nsIStringBundle> bundle = mDownloadManager->mBundle;

  (void)SetState(nsIDownloadManager::DOWNLOAD_FAILED);

  
  nsXPIDLString title;
  nsresult rv = bundle->GetStringFromName(
    NS_LITERAL_STRING("downloadErrorAlertTitle").get(), getter_Copies(title));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsXPIDLString message;
  message = aMessage;
  if (message.IsEmpty()) {
    rv = bundle->GetStringFromName(
      NS_LITERAL_STRING("downloadErrorGeneric").get(), getter_Copies(message));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIWindowMediator> wm =
    do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMWindowInternal> dmWindow;
  rv = wm->GetMostRecentWindow(NS_LITERAL_STRING("Download:Manager").get(),
                               getter_AddRefs(dmWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPromptService> prompter =
    do_GetService("@mozilla.org/embedcomp/prompt-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return prompter->Alert(dmWindow, title, message);
}
