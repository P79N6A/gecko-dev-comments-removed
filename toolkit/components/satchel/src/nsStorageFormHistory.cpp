









































#include "nsStorageFormHistory.h"

#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsICategoryManager.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsCOMArray.h"
#include "mozStorageHelper.h"
#include "mozStorageCID.h"
#include "nsTArray.h"
#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"

#define DB_SCHEMA_VERSION   2
#define DB_FILENAME         NS_LITERAL_STRING("formhistory.sqlite")
#define DB_CORRUPT_FILENAME NS_LITERAL_STRING("formhistory.sqlite.corrupt")

#define PR_HOURS ((PRInt64)60 * 60 * 1000000)


#define MAX_HISTORY_NAME_LEN    200
#define MAX_HISTORY_VALUE_LEN   200

#define MAX_FIELDS_SAVED        100

#define PREF_FORMFILL_BRANCH "browser.formfill."
#define PREF_FORMFILL_ENABLE "enable"

NS_INTERFACE_MAP_BEGIN(nsFormHistory)
  NS_INTERFACE_MAP_ENTRY(nsIFormHistory2)
  NS_INTERFACE_MAP_ENTRY(nsIFormHistoryPrivate)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIFormSubmitObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsFormHistory)
NS_IMPL_RELEASE(nsFormHistory)

PRBool nsFormHistory::gFormHistoryEnabled = PR_FALSE;
PRBool nsFormHistory::gPrefsInitialized = PR_FALSE;
nsFormHistory* nsFormHistory::gFormHistory = nsnull;

nsFormHistory::nsFormHistory()
{
  NS_ASSERTION(!gFormHistory, "nsFormHistory must be used as a service");
  gFormHistory = this;
}

nsFormHistory::~nsFormHistory()
{
  NS_ASSERTION(gFormHistory == this,
               "nsFormHistory must be used as a service");
  gFormHistory = nsnull;
}

nsresult
nsFormHistory::Init()
{
  PRBool doImport;

  nsresult rv = OpenDatabase(&doImport);
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = dbCleanup();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = OpenDatabase(&doImport);
    doImport = PR_FALSE;
  }
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef MOZ_MORKREADER
  if (doImport) {
    
    nsCOMPtr<nsIFile> historyFile;
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(historyFile));
    if (NS_SUCCEEDED(rv)) {
      historyFile->Append(NS_LITERAL_STRING("formhistory.dat"));

      nsCOMPtr<nsIFormHistoryImporter> importer = new nsFormHistoryImporter();
      NS_ENSURE_TRUE(importer, NS_ERROR_OUT_OF_MEMORY);
      importer->ImportFormHistory(historyFile, this);
    }
  }
#endif

  nsCOMPtr<nsIObserverService> service = do_GetService("@mozilla.org/observer-service;1");
  if (service) {
    service->AddObserver(this, NS_EARLYFORMSUBMIT_SUBJECT, PR_TRUE);
    service->AddObserver(this, "idle-daily", PR_TRUE);
    service->AddObserver(this, "formhistory-expire-now", PR_TRUE);
  }

  return NS_OK;
}

 PRBool
nsFormHistory::FormHistoryEnabled()
{
  if (!gPrefsInitialized) {
    nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);

    prefService->GetBranch(PREF_FORMFILL_BRANCH,
                           getter_AddRefs(gFormHistory->mPrefBranch));
    gFormHistory->mPrefBranch->GetBoolPref(PREF_FORMFILL_ENABLE,
                                           &gFormHistoryEnabled);

    nsCOMPtr<nsIPrefBranch2> branchInternal =
      do_QueryInterface(gFormHistory->mPrefBranch);
    branchInternal->AddObserver(PREF_FORMFILL_ENABLE, gFormHistory, PR_TRUE);

    gPrefsInitialized = PR_TRUE;
  }

  return gFormHistoryEnabled;
}





NS_IMETHODIMP
nsFormHistory::GetHasEntries(PRBool *aHasEntries)
{
  mozStorageStatementScoper scope(mDBSelectEntries);

  PRBool hasMore;
  *aHasEntries = NS_SUCCEEDED(mDBSelectEntries->ExecuteStep(&hasMore)) && hasMore;
  return NS_OK;
}

NS_IMETHODIMP
nsFormHistory::AddEntry(const nsAString &aName, const nsAString &aValue)
{
  
  nsresult rv;
  nsCOMPtr<nsIPrivateBrowsingService> pbs =
    do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (pbs) {
    PRBool inPrivateBrowsing = PR_TRUE;
    rv = pbs->GetPrivateBrowsingEnabled(&inPrivateBrowsing);
    if (NS_FAILED(rv))
      inPrivateBrowsing = PR_TRUE; 
    if (inPrivateBrowsing)
      return NS_OK;
  }

  if (!FormHistoryEnabled())
    return NS_OK;

  PRInt64 existingID = GetExistingEntryID(aName, aValue);

  if (existingID != -1) {
    mozStorageStatementScoper scope(mDBUpdateEntry);
    
    rv = mDBUpdateEntry->BindInt64Parameter(0, PR_Now());
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mDBUpdateEntry->BindInt64Parameter(1, existingID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBUpdateEntry->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    PRInt64 now = PR_Now();

    mozStorageStatementScoper scope(mDBInsertNameValue);
    
    rv = mDBInsertNameValue->BindStringParameter(0, aName);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mDBInsertNameValue->BindStringParameter(1, aValue);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mDBInsertNameValue->BindInt32Parameter(2, 1);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mDBInsertNameValue->BindInt64Parameter(3, now);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mDBInsertNameValue->BindInt64Parameter(4, now);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBInsertNameValue->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


PRInt64
nsFormHistory::GetExistingEntryID (const nsAString &aName, 
                                   const nsAString &aValue)
{
  mozStorageStatementScoper scope(mDBFindEntry);

  nsresult rv = mDBFindEntry->BindStringParameter(0, aName);
  NS_ENSURE_SUCCESS(rv, -1);

  rv = mDBFindEntry->BindStringParameter(1, aValue);
  NS_ENSURE_SUCCESS(rv, -1);

  PRBool hasMore;
  rv = mDBFindEntry->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, -1);

  PRInt64 ID = -1;
  if (hasMore) {
    mDBFindEntry->GetInt64(0, &ID);
    NS_ENSURE_SUCCESS(rv, -1);
  }

  return ID;
}

NS_IMETHODIMP
nsFormHistory::EntryExists(const nsAString &aName, 
                           const nsAString &aValue, PRBool *_retval)
{
  PRInt64 ID = GetExistingEntryID(aName, aValue);
  *_retval = ID != -1;
  return NS_OK;
}

NS_IMETHODIMP
nsFormHistory::NameExists(const nsAString &aName, PRBool *_retval)
{
  mozStorageStatementScoper scope(mDBFindEntryByName);

  nsresult rv = mDBFindEntryByName->BindStringParameter(0, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  *_retval = (NS_SUCCEEDED(mDBFindEntryByName->ExecuteStep(&hasMore)) &&
              hasMore);
  return NS_OK;
}

NS_IMETHODIMP
nsFormHistory::RemoveEntry(const nsAString &aName, const nsAString &aValue)
{
  nsCOMPtr<mozIStorageStatement> dbDelete;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_formhistory WHERE fieldname=?1 AND value=?2"),
                                         getter_AddRefs(dbDelete));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = dbDelete->BindStringParameter(0, aName);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = dbDelete->BindStringParameter(1, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  return dbDelete->Execute();
}

NS_IMETHODIMP
nsFormHistory::RemoveEntriesForName(const nsAString &aName)
{
  nsCOMPtr<mozIStorageStatement> dbDelete;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_formhistory WHERE fieldname=?1"),
                                         getter_AddRefs(dbDelete));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = dbDelete->BindStringParameter(0, aName);
  NS_ENSURE_SUCCESS(rv,rv);

  return dbDelete->Execute();
}

NS_IMETHODIMP
nsFormHistory::RemoveAllEntries()
{
  nsCOMPtr<mozIStorageStatement> dbDeleteAll;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_formhistory"),
                                         getter_AddRefs(dbDeleteAll));
  NS_ENSURE_SUCCESS(rv,rv);

  
  nsCOMPtr<nsIFile> oldFormHistoryFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(oldFormHistoryFile));
  if (NS_FAILED(rv)) return rv;

  rv = oldFormHistoryFile->Append(NS_LITERAL_STRING("formhistory.dat"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool fileExists;
  if (NS_SUCCEEDED(oldFormHistoryFile->Exists(&fileExists)) && fileExists) {
    rv = oldFormHistoryFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return dbDeleteAll->Execute();
}


NS_IMETHODIMP
nsFormHistory::RemoveEntriesByTimeframe(PRInt64 aStartTime, PRInt64 aEndTime)
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_formhistory "
    "WHERE firstUsed >= ?1 "
    "AND firstUsed <= ?2"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = stmt->BindInt64Parameter(0, aStartTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt64Parameter(1, aEndTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsFormHistory::GetDBConnection(mozIStorageConnection **aResult)
{
  NS_ADDREF(*aResult = mDBConn);
  return NS_OK;
}




NS_IMETHODIMP
nsFormHistory::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData) 
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    mPrefBranch->GetBoolPref(PREF_FORMFILL_ENABLE, &gFormHistoryEnabled);
  } else if (!strcmp(aTopic, "idle-daily") ||
             !strcmp(aTopic, "formhistory-expire-now")) {
      ExpireOldEntries();
  }

  return NS_OK;
}




NS_IMETHODIMP
nsFormHistory::Notify(nsIDOMHTMLFormElement* formElt, nsIDOMWindowInternal* aWindow, nsIURI* aActionURL, PRBool* aCancelSubmit)
{
  if (!FormHistoryEnabled())
    return NS_OK;

  NS_NAMED_LITERAL_STRING(kAutoComplete, "autocomplete");
  nsAutoString autocomplete;
  formElt->GetAttribute(kAutoComplete, autocomplete);
  if (autocomplete.LowerCaseEqualsLiteral("off"))
    return NS_OK;

  nsCOMPtr<nsIDOMHTMLCollection> elts;
  formElt->GetElements(getter_AddRefs(elts));

  PRUint32 savedCount = 0;
  PRUint32 length;
  elts->GetLength(&length);
  if (length == 0)
    return NS_OK;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<nsIDOMNode> node;
    elts->Item(i, getter_AddRefs(node));
    nsCOMPtr<nsIDOMHTMLInputElement> inputElt = do_QueryInterface(node);
    if (inputElt) {
      
      nsAutoString type;
      inputElt->GetType(type);
      if (!type.LowerCaseEqualsLiteral("text"))
        continue;

      
      

      nsAutoString autocomplete;
      inputElt->GetAttribute(kAutoComplete, autocomplete);
      if (!autocomplete.LowerCaseEqualsLiteral("off")) {
        
        nsAutoString value;
        inputElt->GetValue(value);
        value.Trim(" \t", PR_TRUE, PR_TRUE);
        if (!value.IsEmpty()) {
          
          
          nsAutoString defaultValue;
          inputElt->GetDefaultValue(defaultValue);
          if (value.Equals(defaultValue))
            continue;

          nsAutoString name;
          inputElt->GetName(name);
          if (name.IsEmpty())
            inputElt->GetId(name);

          if (name.IsEmpty())
            continue;
          if (name.Length() > MAX_HISTORY_NAME_LEN ||
              value.Length() > MAX_HISTORY_VALUE_LEN)
            continue;
          if (savedCount++ >= MAX_FIELDS_SAVED)
            break;
          AddEntry(name, value);
        }
      }
    }
  }

  return transaction.Commit();
}

nsresult
nsFormHistory::ExpireOldEntries()
{
  
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 expireDays;
  rv = prefBranch->GetIntPref("browser.formfill.expire_days", &expireDays);
  if (NS_FAILED(rv)) {
    PRInt32 expireDaysMin = 0;
    rv = prefBranch->GetIntPref("browser.history_expire_days", &expireDays);
    NS_ENSURE_SUCCESS(rv, rv);
    prefBranch->GetIntPref("browser.history_expire_days_min", &expireDaysMin);
    
    
    if (expireDays && expireDays < expireDaysMin)
      expireDays = expireDaysMin;
  }
  PRInt64 expireTime = PR_Now() - expireDays * 24 * PR_HOURS;


  PRInt32 beginningCount = CountAllEntries();

  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "DELETE FROM moz_formhistory WHERE lastUsed<=?1"),
          getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv,rv);
  rv = stmt->BindInt64Parameter(0, expireTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 endingCount = CountAllEntries();

  
  
  
  
  if (beginningCount - endingCount > 500) {
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("VACUUM"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

PRInt32
nsFormHistory::CountAllEntries()
{
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
                  "SELECT COUNT(*) FROM moz_formhistory"),
                  getter_AddRefs(stmt));

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, 0);

  PRInt32 count = 0;
  if (hasResult)
    count = stmt->AsInt32(0);

  return count;
}

nsresult
nsFormHistory::CreateTable()
{
  nsresult rv;
  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
         "CREATE TABLE moz_formhistory ("
           "id INTEGER PRIMARY KEY, fieldname TEXT NOT NULL, "
           "value TEXT NOT NULL, timesUsed INTEGER, "
           "firstUsed INTEGER, lastUsed INTEGER)"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE INDEX moz_formhistory_index ON moz_formhistory (fieldname)"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE INDEX moz_formhistory_lastused_index ON moz_formhistory (lastUsed)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->SetSchemaVersion(DB_SCHEMA_VERSION);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsFormHistory::CreateStatements()
{
  nsresult rv;
  rv  = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
         "SELECT * FROM moz_formhistory"),
         getter_AddRefs(mDBSelectEntries));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
         "SELECT id FROM moz_formhistory WHERE fieldname=?1 AND value=?2"),
         getter_AddRefs(mDBFindEntry));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
         "SELECT * FROM moz_formhistory WHERE fieldname=?1"),
         getter_AddRefs(mDBFindEntryByName));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "INSERT INTO moz_formhistory (fieldname, value, timesUsed, "
        "firstUsed, lastUsed) VALUES (?1, ?2, ?3, ?4, ?5)"),
        getter_AddRefs(mDBInsertNameValue));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "UPDATE moz_formhistory "
        "SET timesUsed=timesUsed + 1, lastUsed=?1 "
        "WHERE id = ?2"),
        getter_AddRefs(mDBUpdateEntry));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsFormHistory::OpenDatabase(PRBool *aDoImport)
{
  
  nsresult rv;
  mStorageService = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> formHistoryFile;
  rv = GetDatabaseFile(getter_AddRefs(formHistoryFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mStorageService->OpenDatabase(formHistoryFile, getter_AddRefs(mDBConn));
  NS_ENSURE_SUCCESS(rv, rv);

  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  PRBool exists;
  mDBConn->TableExists(NS_LITERAL_CSTRING("moz_formhistory"), &exists);
  if (!exists) {
    *aDoImport = PR_TRUE;
    rv = CreateTable();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    *aDoImport = PR_FALSE;
  }

  
  rv = dbMigrate();
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  transaction.Commit();

  rv = CreateStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
  



nsresult
nsFormHistory::dbMigrate()
{
  PRInt32 schemaVersion;
  nsresult rv = mDBConn->GetSchemaVersion(&schemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  switch (schemaVersion) {
    case 0:
      rv = MigrateToVersion1();
      NS_ENSURE_SUCCESS(rv, rv);
      
    case 1:
      rv = MigrateToVersion2();
      NS_ENSURE_SUCCESS(rv, rv);
      
    case DB_SCHEMA_VERSION:
      
      break;

    
    
    default:
      
      
      if(!dbAreExpectedColumnsPresent())
        return NS_ERROR_FILE_CORRUPTED;

      
      
      rv = mDBConn->SetSchemaVersion(DB_SCHEMA_VERSION);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
  }

  return NS_OK;
}








nsresult
nsFormHistory::MigrateToVersion1()
{
  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
                  "SELECT timesUsed, firstUsed, lastUsed FROM moz_formhistory"),
                  getter_AddRefs(stmt));

  PRBool columnsExist = !!NS_SUCCEEDED(rv);

  if (!columnsExist) {
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_formhistory ADD COLUMN timesUsed INTEGER"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_formhistory ADD COLUMN firstUsed INTEGER"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_formhistory ADD COLUMN lastUsed INTEGER"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  
  nsCOMPtr<mozIStorageStatement> addDefaultValues;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
         "UPDATE moz_formhistory "
         "SET timesUsed=1, firstUsed=?1, lastUsed=?1 "
         "WHERE timesUsed isnull OR firstUsed isnull OR lastUsed isnull"),
         getter_AddRefs(addDefaultValues));
  rv = addDefaultValues->BindInt64Parameter(0, PR_Now() - 24 * PR_HOURS);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = addDefaultValues->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->SetSchemaVersion(1);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}








nsresult
nsFormHistory::MigrateToVersion2()
{
  nsresult rv;

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE IF EXISTS moz_dummy_table"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS moz_formhistory_lastused_index ON moz_formhistory (lastUsed)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->SetSchemaVersion(2);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsFormHistory::GetDatabaseFile(nsIFile** aFile)
{
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, aFile);
  NS_ENSURE_SUCCESS(rv, rv);
  return (*aFile)->Append(DB_FILENAME);
}








nsresult
nsFormHistory::dbCleanup()
{
  nsCOMPtr<nsIFile> dbFile;
  nsresult rv = GetDatabaseFile(getter_AddRefs(dbFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> backupFile;
  NS_ENSURE_TRUE(mStorageService, NS_ERROR_NOT_AVAILABLE);
  rv = mStorageService->BackupDatabaseFile(dbFile, DB_CORRUPT_FILENAME,
                                           nsnull, getter_AddRefs(backupFile));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mDBConn)
    mDBConn->Close();

  rv = dbFile->Remove(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




PRBool
nsFormHistory::dbAreExpectedColumnsPresent()
{
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
                  "SELECT fieldname, value, timesUsed, firstUsed, lastUsed "
                  "FROM moz_formhistory"), getter_AddRefs(stmt));
  return NS_SUCCEEDED(rv) ? PR_TRUE : PR_FALSE;
}


#ifdef MOZ_MORKREADER


enum {
  kNameColumn,
  kValueColumn,
  kColumnCount 
};

static const char * const gColumnNames[] = {
  "Name", "Value"
};

struct FormHistoryImportClosure
{
  FormHistoryImportClosure(nsMorkReader *aReader, nsIFormHistory2 *aFormHistory)
    : reader(aReader), formHistory(aFormHistory), byteOrderColumn(-1),
      swapBytes(PR_FALSE)
  {
    for (PRUint32 i = 0; i < kColumnCount; ++i) {
      columnIndexes[i] = -1;
    }
  }

  
  const nsMorkReader *reader;
  nsIFormHistory2 *formHistory;

  
  PRInt32 columnIndexes[kColumnCount];
  PRInt32 byteOrderColumn;

  
  PRPackedBool swapBytes;
};




static void SwapBytes(PRUnichar* aBuffer)
{
  for (PRUnichar *b = aBuffer; *b; b++)
  {
    PRUnichar c = *b;
    *b = (0xff & (c >> 8)) | (c << 8);
  }
}


 PLDHashOperator
nsFormHistoryImporter::AddToFormHistoryCB(const nsCSubstring &aRowID,
                                          const nsTArray<nsCString> *aValues,
                                          void *aData)
{
  FormHistoryImportClosure *data = static_cast<FormHistoryImportClosure*>
                                              (aData);
  const nsMorkReader *reader = data->reader;
  nsCString values[kColumnCount];
  const PRUnichar* valueStrings[kColumnCount];
  PRUint32 valueLengths[kColumnCount];
  const PRInt32 *columnIndexes = data->columnIndexes;
  PRInt32 i;

  

  for (i = 0; i < kColumnCount; ++i) {
    if (columnIndexes[i] == -1) {
      
      continue;
    }

    values[i] = (*aValues)[columnIndexes[i]];
    reader->NormalizeValue(values[i]);

    PRUint32 length;
    const char *bytes;
    if (values[i].IsEmpty()) {
      bytes = "\0";
      length = 0;
    } else {
      length = values[i].Length() / 2;

      
      
      values[i].Append('\0');

      
      if (data->swapBytes) {
        SwapBytes(reinterpret_cast<PRUnichar*>(values[i].BeginWriting()));
      }
      bytes = values[i].get();
    }
    valueStrings[i] = reinterpret_cast<const PRUnichar*>(bytes);
    valueLengths[i] = length;
  }

  data->formHistory->AddEntry(nsDependentString(valueStrings[kNameColumn],
                                                valueLengths[kNameColumn]),
                              nsDependentString(valueStrings[kValueColumn],
                                                valueLengths[kValueColumn]));
  return PL_DHASH_NEXT;
}

NS_IMPL_ISUPPORTS1(nsFormHistoryImporter, nsIFormHistoryImporter)

NS_IMETHODIMP
nsFormHistoryImporter::ImportFormHistory(nsIFile *aFile,
                                         nsIFormHistory2 *aFormHistory)
{
  
  PRBool exists;
  aFile->Exists(&exists);
  if (!exists) {
    return NS_OK;
  }
  
  nsMorkReader reader;
  nsresult rv = reader.Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = reader.Read(aFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  FormHistoryImportClosure data(&reader, aFormHistory);
  const nsTArray<nsMorkReader::MorkColumn> columns = reader.GetColumns();
  for (PRUint32 i = 0; i < columns.Length(); ++i) {
    const nsCSubstring &name = columns[i].name;
    for (PRUint32 j = 0; j < kColumnCount; ++j) {
      if (name.Equals(gColumnNames[j])) {
        data.columnIndexes[j] = i;
        break;
      }
    }
    if (name.EqualsLiteral("ByteOrder")) {
      data.byteOrderColumn = i;
    }
  }

  
  const nsTArray<nsCString> *metaRow = reader.GetMetaRow();
  if (metaRow && data.byteOrderColumn != -1) {
    const nsCString &byteOrder = (*metaRow)[data.byteOrderColumn];
    
    
    
    
    
    nsCAutoString byteOrderValue(byteOrder);
    reader.NormalizeValue(byteOrderValue);
#ifdef IS_LITTLE_ENDIAN
    data.swapBytes = byteOrderValue.EqualsLiteral("BBBB");
#else
    data.swapBytes = byteOrderValue.EqualsLiteral("llll");
#endif
  }
#if defined(XP_MACOSX) && defined(IS_LITTLE_ENDIAN)
  
  
  
  
  
  
  
  
  else {
    data.swapBytes = PR_TRUE;
  }
#endif

  
  nsCOMPtr<nsIFormHistoryPrivate> fhPrivate = do_QueryInterface(aFormHistory);
  NS_ENSURE_TRUE(fhPrivate, NS_ERROR_FAILURE);

  mozIStorageConnection *conn = fhPrivate->GetStorageConnection();
  NS_ENSURE_TRUE(conn, NS_ERROR_NOT_INITIALIZED);
  mozStorageTransaction transaction(conn, PR_FALSE);

  reader.EnumerateRows(AddToFormHistoryCB, &data);
  return transaction.Commit();
}
#endif
