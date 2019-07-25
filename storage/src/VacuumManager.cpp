






































#include "VacuumManager.h"

#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "nsIObserverService.h"
#include "nsPrintfCString.h"
#include "nsIFile.h"
#include "nsThreadUtils.h"
#include "prlog.h"

#include "mozStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozIStorageAsyncStatement.h"
#include "mozIStoragePendingStatement.h"
#include "mozIStorageError.h"

#define OBSERVER_TOPIC_IDLE_DAILY "idle-daily"
#define OBSERVER_TOPIC_XPCOM_SHUTDOWN "xpcom-shutdown"


#define OBSERVER_TOPIC_HEAVY_IO "heavy-io-task"
#define OBSERVER_DATA_VACUUM_BEGIN NS_LITERAL_STRING("vacuum-begin")
#define OBSERVER_DATA_VACUUM_END NS_LITERAL_STRING("vacuum-end")



#define PREF_VACUUM_BRANCH "storage.vacuum.last."


#define VACUUM_INTERVAL_SECONDS 30 * 86400 // 30 days.

#ifdef PR_LOGGING
extern PRLogModuleInfo *gStorageLog;
#endif

namespace mozilla {
namespace storage {

namespace {




class BaseCallback : public mozIStorageStatementCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK
  BaseCallback() {}
protected:
  virtual ~BaseCallback() {}
};

NS_IMETHODIMP
BaseCallback::HandleError(mozIStorageError *aError)
{
#ifdef DEBUG
  PRInt32 result;
  nsresult rv = aError->GetResult(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString message;
  rv = aError->GetMessage(message);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString warnMsg;
  warnMsg.AppendLiteral("An error occured during async execution: ");
  warnMsg.AppendInt(result);
  warnMsg.AppendLiteral(" ");
  warnMsg.Append(message);
  NS_WARNING(warnMsg.get());
#endif
  return NS_OK;
}

NS_IMETHODIMP
BaseCallback::HandleResult(mozIStorageResultSet *aResultSet)
{
  
  return NS_OK;
}

NS_IMETHODIMP
BaseCallback::HandleCompletion(PRUint16 aReason)
{
  
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(
  BaseCallback
, mozIStorageStatementCallback
)




class Vacuumer : public BaseCallback
{
public:
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK

  Vacuumer(mozIStorageVacuumParticipant *aParticipant);

  bool execute();
  nsresult notifyCompletion(bool aSucceeded);

private:
  nsCOMPtr<mozIStorageVacuumParticipant> mParticipant;
  nsCString mDBFilename;
  nsCOMPtr<mozIStorageConnection> mDBConn;
};








class NotifyCallback : public BaseCallback
{
public:
  NS_IMETHOD HandleCompletion(PRUint16 aReason);

  NotifyCallback(Vacuumer *aVacuumer, bool aVacuumSucceeded);

private:
  nsCOMPtr<Vacuumer> mVacuumer;
  bool mVacuumSucceeded;
};

NotifyCallback::NotifyCallback(Vacuumer *aVacuumer,
                               bool aVacuumSucceeded)
  : mVacuumer(aVacuumer)
  , mVacuumSucceeded(aVacuumSucceeded)
{
}

NS_IMETHODIMP
NotifyCallback::HandleCompletion(PRUint16 aReason)
{
  
  nsresult rv = mVacuumer->notifyCompletion(mVacuumSucceeded);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}




Vacuumer::Vacuumer(mozIStorageVacuumParticipant *aParticipant)
  : mParticipant(aParticipant)
{
}

bool
Vacuumer::execute()
{
  NS_PRECONDITION(NS_IsMainThread(), "Must be running on the main thread!");

  
  nsresult rv = mParticipant->GetDatabaseConnection(getter_AddRefs(mDBConn));
  NS_ENSURE_SUCCESS(rv, false);
  PRBool ready;
  if (!mDBConn || NS_FAILED(mDBConn->GetConnectionReady(&ready)) || !ready) {
    NS_WARNING(NS_LITERAL_CSTRING("Unable to get a connection to vacuum database").get());
    return false;
  }

  
  
  
  PRInt32 expectedPageSize = 0;
  rv = mParticipant->GetExpectedDatabasePageSize(&expectedPageSize);
  if (NS_FAILED(rv) || expectedPageSize < 512 || expectedPageSize > 65536) {
    NS_WARNING("Invalid page size requested for database, will use default ");
    NS_WARNING(mDBFilename.get());
    expectedPageSize = mozIStorageConnection::DEFAULT_PAGE_SIZE;
  }

  bool canOptimizePageSize = false;
  {
    nsCOMPtr<mozIStorageStatement> stmt;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "PRAGMA page_size"
    ), getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, false);
    PRBool hasResult;
    rv = stmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, false);
    NS_ENSURE_TRUE(hasResult, false);
    PRInt32 currentPageSize;
    rv = stmt->GetInt32(0, &currentPageSize);
    NS_ENSURE_SUCCESS(rv, false);
    NS_ASSERTION(currentPageSize > 0, "Got invalid page size value?");
    if (currentPageSize != expectedPageSize) {
      
      
      
      nsCAutoString journalMode;
      {
        nsCOMPtr<mozIStorageStatement> stmt;
        rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "PRAGMA journal_mode"
        ), getter_AddRefs(stmt));
        NS_ENSURE_SUCCESS(rv, false);
        PRBool hasResult;
        rv = stmt->ExecuteStep(&hasResult);
        NS_ENSURE_SUCCESS(rv, false);
        NS_ENSURE_TRUE(hasResult, false);
        rv = stmt->GetUTF8String(0, journalMode);
        NS_ENSURE_SUCCESS(rv, false);
      }
      canOptimizePageSize = !journalMode.EqualsLiteral("wal");
    }
  }

  
  
  nsCOMPtr<nsIFile> databaseFile;
  mDBConn->GetDatabaseFile(getter_AddRefs(databaseFile));
  if (!databaseFile) {
    NS_WARNING("Trying to vacuum a in-memory database!");
    return false;
  }
  nsAutoString databaseFilename;
  rv = databaseFile->GetLeafName(databaseFilename);
  NS_ENSURE_SUCCESS(rv, false);
  mDBFilename = NS_ConvertUTF16toUTF8(databaseFilename);
  NS_ASSERTION(!mDBFilename.IsEmpty(), "Database filename cannot be empty");

  
  PRInt32 now = static_cast<PRInt32>(PR_Now() / PR_USEC_PER_SEC);
  PRInt32 lastVacuum;
  nsCAutoString prefName(PREF_VACUUM_BRANCH);
  prefName += mDBFilename;
  rv = Preferences::GetInt(prefName.get(), &lastVacuum);
  if (NS_SUCCEEDED(rv) && (now - lastVacuum) < VACUUM_INTERVAL_SECONDS &&
      !canOptimizePageSize) {
    
    return false;
  }

  
  
  
  PRBool vacuumGranted = PR_FALSE;
  rv = mParticipant->OnBeginVacuum(&vacuumGranted);
  NS_ENSURE_SUCCESS(rv, false);
  if (!vacuumGranted) {
    return false;
  }

  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    (void)os->NotifyObservers(nsnull, OBSERVER_TOPIC_HEAVY_IO,
                              OBSERVER_DATA_VACUUM_BEGIN.get());
  }

  if (canOptimizePageSize) {
    nsCOMPtr<mozIStorageAsyncStatement> pageSizeStmt;
    rv = mDBConn->CreateAsyncStatement(nsPrintfCString(
      "PRAGMA page_size = %ld", expectedPageSize
    ), getter_AddRefs(pageSizeStmt));
    NS_ENSURE_SUCCESS(rv, false);
    nsCOMPtr<BaseCallback> callback = new BaseCallback();
    NS_ENSURE_TRUE(callback, false);
    nsCOMPtr<mozIStoragePendingStatement> ps;
    rv = pageSizeStmt->ExecuteAsync(callback, getter_AddRefs(ps));
    NS_ENSURE_SUCCESS(rv, false);
  }

  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "VACUUM"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, false);

  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = stmt->ExecuteAsync(this, getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}




NS_IMETHODIMP
Vacuumer::HandleError(mozIStorageError *aError)
{
#ifdef DEBUG
  PRInt32 result;
  nsresult rv = aError->GetResult(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString message;
  rv = aError->GetMessage(message);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString warnMsg;
  warnMsg.AppendLiteral("Unable to vacuum database: ");
  warnMsg.Append(mDBFilename);
  warnMsg.AppendLiteral(" - ");
  warnMsg.AppendInt(result);
  warnMsg.AppendLiteral(" ");
  warnMsg.Append(message);
  NS_WARNING(warnMsg.get());
#endif

#ifdef PR_LOGGING
  {
    PRInt32 result;
    nsresult rv = aError->GetResult(&result);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString message;
    rv = aError->GetMessage(message);
    NS_ENSURE_SUCCESS(rv, rv);
    PR_LOG(gStorageLog, PR_LOG_ERROR,
           ("Vacuum failed with error: %d '%s'. Database was: '%s'",
            result, message.get(), mDBFilename.get()));
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
Vacuumer::HandleResult(mozIStorageResultSet *aResultSet)
{
  NS_NOTREACHED("Got a resultset from a vacuum?");
  return NS_OK;
}

NS_IMETHODIMP
Vacuumer::HandleCompletion(PRUint16 aReason)
{
  if (aReason == REASON_FINISHED) {
    
    PRInt32 now = static_cast<PRInt32>(PR_Now() / PR_USEC_PER_SEC);
    NS_ASSERTION(!mDBFilename.IsEmpty(), "Database filename cannot be empty");
    nsCAutoString prefName(PREF_VACUUM_BRANCH);
    prefName += mDBFilename;
    (void)Preferences::SetInt(prefName.get(), now);
  }

  notifyCompletion(aReason == REASON_FINISHED);

  return NS_OK;
}

nsresult
Vacuumer::notifyCompletion(bool aSucceeded)
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    os->NotifyObservers(nsnull, OBSERVER_TOPIC_HEAVY_IO,
                        OBSERVER_DATA_VACUUM_END.get());
  }

  nsresult rv = mParticipant->OnEndVacuum(aSucceeded);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

} 




NS_IMPL_ISUPPORTS1(
  VacuumManager
, nsIObserver
)

VacuumManager *
VacuumManager::gVacuumManager = nsnull;

VacuumManager *
VacuumManager::getSingleton()
{
  if (gVacuumManager) {
    NS_ADDREF(gVacuumManager);
    return gVacuumManager;
  }
  gVacuumManager = new VacuumManager();
  if (gVacuumManager) {
    NS_ADDREF(gVacuumManager);
  }
  return gVacuumManager;
}

VacuumManager::VacuumManager()
  : mParticipants("vacuum-participant")
{
  NS_ASSERTION(!gVacuumManager,
               "Attempting to create two instances of the service!");
  gVacuumManager = this;
}

VacuumManager::~VacuumManager()
{
  
  
  NS_ASSERTION(gVacuumManager == this,
               "Deleting a non-singleton instance of the service");
  if (gVacuumManager == this) {
    gVacuumManager = nsnull;
  }
}




NS_IMETHODIMP
VacuumManager::Observe(nsISupports *aSubject,
                       const char *aTopic,
                       const PRUnichar *aData)
{
  if (strcmp(aTopic, OBSERVER_TOPIC_IDLE_DAILY) == 0) {
    
    
    const nsCOMArray<mozIStorageVacuumParticipant> &entries =
      mParticipants.GetEntries();
    
    
    static const char* kPrefName = PREF_VACUUM_BRANCH "index";
    PRInt32 startIndex = Preferences::GetInt(kPrefName, 0);
    if (startIndex >= entries.Count()) {
      startIndex = 0;
    }
    PRInt32 index;
    for (index = startIndex; index < entries.Count(); ++index) {
      nsCOMPtr<Vacuumer> vacuum = new Vacuumer(entries[index]);
      
      if (vacuum->execute()) {
        break;
      }
    }
    (void)Preferences::SetInt(kPrefName, index);
  }

  return NS_OK;
}

} 
} 
