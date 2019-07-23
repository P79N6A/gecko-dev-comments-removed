










































#include "nsStorageFormHistory.h"

#include "plbase64.h"
#include "prmem.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsICategoryManager.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIURI.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsCOMArray.h"
#include "mozStorageHelper.h"
#include "mozStorageCID.h"
#include "nsTArray.h"
#include "nsIMutableArray.h"
#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"

#define DB_SCHEMA_VERSION   3
#define DB_FILENAME         NS_LITERAL_STRING("formhistory.sqlite")
#define DB_CORRUPT_FILENAME NS_LITERAL_STRING("formhistory.sqlite.corrupt")

#define PR_HOURS ((PRInt64)60 * 60 * 1000000)


#define MAX_HISTORY_NAME_LEN    200
#define MAX_HISTORY_VALUE_LEN   200

#define MAX_FIELDS_SAVED        100

#define PREF_FORMFILL_BRANCH "browser.formfill."
#define PREF_FORMFILL_ENABLE "enable"
#define PREF_FORMFILL_SAVE_HTTPS_FORMS "saveHttpsForms"



#define DEFAULT_EXPIRE_DAYS 180

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

PRBool nsFormHistory::gFormHistoryEnabled = PR_TRUE;
PRBool nsFormHistory::gSaveHttpsForms = PR_TRUE;
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

  mUUIDService = do_GetService("@mozilla.org/uuid-generator;1");
  NS_ENSURE_TRUE(mUUIDService, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = OpenDatabase(&doImport);
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = dbCleanup();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = OpenDatabase(&doImport);
    doImport = PR_FALSE;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  mObserverService = do_GetService("@mozilla.org/observer-service;1");
  if (mObserverService) {
    mObserverService->AddObserver(this, NS_EARLYFORMSUBMIT_SUBJECT, PR_TRUE);
    mObserverService->AddObserver(this, "idle-daily", PR_TRUE);
    mObserverService->AddObserver(this, "formhistory-expire-now", PR_TRUE);
  }

  return NS_OK;
}

 nsresult
nsFormHistory::InitPrefs()
{
  nsCOMPtr<nsIPrefService> prefService =
      do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(prefService, NS_ERROR_FAILURE);
  prefService->GetBranch(PREF_FORMFILL_BRANCH,
                         getter_AddRefs(gFormHistory->mPrefBranch));
  NS_ENSURE_TRUE(gFormHistory->mPrefBranch, NS_ERROR_FAILURE);
  gFormHistory->mPrefBranch->GetBoolPref(PREF_FORMFILL_ENABLE,
                                         &gFormHistoryEnabled);
  gFormHistory->mPrefBranch->GetBoolPref(PREF_FORMFILL_SAVE_HTTPS_FORMS,
                                         &gSaveHttpsForms);
  nsCOMPtr<nsIPrefBranch2> branchInternal =
      do_QueryInterface(gFormHistory->mPrefBranch);
  NS_ENSURE_TRUE(branchInternal, NS_ERROR_FAILURE);
  branchInternal->AddObserver("", gFormHistory, PR_TRUE);
  gPrefsInitialized = PR_TRUE;
  return NS_OK;
}

 PRBool
nsFormHistory::SaveHttpsForms()
{
  if (!gPrefsInitialized) {
    InitPrefs();
  }
  return gSaveHttpsForms;
}

 PRBool
nsFormHistory::FormHistoryEnabled()
{
  if (!gPrefsInitialized)
    InitPrefs();
  return gFormHistoryEnabled;
}

nsresult
nsFormHistory::GenerateGUID(nsACString &guidString) {
  nsID rawguid;
  nsresult rv = mUUIDService->GenerateUUIDInPlace(&rawguid);
  NS_ENSURE_SUCCESS(rv, rv);

  
  char *b64 = PL_Base64Encode(reinterpret_cast<const char *>(&rawguid), 12, nsnull);
  if (!b64)
    return NS_ERROR_OUT_OF_MEMORY;

  guidString.Assign(b64);
  PR_Free(b64);
  return NS_OK;
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

  nsAutoString existingGUID;
  PRInt64 existingID = GetExistingEntryID(aName, aValue, existingGUID);

  if (existingID != -1) {
    mozStorageStatementScoper scope(mDBUpdateEntry);
    
    rv = mDBUpdateEntry->BindInt64Parameter(0, PR_Now());
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mDBUpdateEntry->BindInt64Parameter(1, existingID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBUpdateEntry->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    SendNotification(NS_LITERAL_STRING("modifyEntry"), aName, aValue, existingGUID);
  } else {
    nsCAutoString guid;
    rv = GenerateGUID(guid);
    NS_ENSURE_SUCCESS(rv, rv);

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
    
    rv = mDBInsertNameValue->BindUTF8StringParameter(5, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBInsertNameValue->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    SendNotification(NS_LITERAL_STRING("addEntry"), aName, aValue, NS_ConvertUTF8toUTF16(guid));
  }
  return NS_OK;
}


PRInt64
nsFormHistory::GetExistingEntryID (const nsAString &aName, 
                                   const nsAString &aValue,
                                   nsAString &aGuid)
{
  mozStorageStatementScoper scope(mDBFindEntry);

  nsresult rv = mDBFindEntry->BindStringParameter(0, aName);
  NS_ENSURE_SUCCESS(rv, -1);

  rv = mDBFindEntry->BindStringParameter(1, aValue);
  NS_ENSURE_SUCCESS(rv, -1);

  PRBool hasMore;
  rv = mDBFindEntry->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, -1);

  nsCAutoString guid;
  PRInt64 ID = -1;
  if (hasMore) {
    rv = mDBFindEntry->GetInt64(0, &ID);
    NS_ENSURE_SUCCESS(rv, -1);
    rv = mDBFindEntry->GetUTF8String(1, guid);
    NS_ENSURE_SUCCESS(rv, -1);
    CopyUTF8toUTF16(guid, aGuid);
  }

  return ID;
}


PRInt64
nsFormHistory::GetExistingEntryID (const nsAString &aName,
                                   const nsAString &aValue)
{
  nsString guid;
  return GetExistingEntryID(aName, aValue, guid);
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
  nsAutoString existingGUID;
  PRInt64 existingID = GetExistingEntryID(aName, aValue, existingGUID);

  SendNotification(NS_LITERAL_STRING("before-removeEntry"), aName, aValue, existingGUID);

  nsCOMPtr<mozIStorageStatement> dbDelete;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_formhistory WHERE id=?1"),
                                         getter_AddRefs(dbDelete));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = dbDelete->BindInt64Parameter(0, existingID);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = dbDelete->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  SendNotification(NS_LITERAL_STRING("removeEntry"), aName, aValue, existingGUID);

  return NS_OK;
}

NS_IMETHODIMP
nsFormHistory::RemoveEntriesForName(const nsAString &aName)
{
  SendNotification(NS_LITERAL_STRING("before-removeEntriesForName"), aName);

  nsCOMPtr<mozIStorageStatement> dbDelete;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_formhistory WHERE fieldname=?1"),
                                         getter_AddRefs(dbDelete));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = dbDelete->BindStringParameter(0, aName);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = dbDelete->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  SendNotification(NS_LITERAL_STRING("removeEntriesForName"), aName);

  return NS_OK;
}

NS_IMETHODIMP
nsFormHistory::RemoveAllEntries()
{
  SendNotification(NS_LITERAL_STRING("before-removeAllEntries"), (nsISupports*)nsnull);

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

  rv = dbDeleteAll->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  SendNotification(NS_LITERAL_STRING("removeAllEntries"), (nsISupports*)nsnull);

  return NS_OK;
}


NS_IMETHODIMP
nsFormHistory::RemoveEntriesByTimeframe(PRInt64 aStartTime, PRInt64 aEndTime)
{
  SendNotification(NS_LITERAL_STRING("before-removeEntriesByTimeframe"), aStartTime, aEndTime);

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

  SendNotification(NS_LITERAL_STRING("removeEntriesByTimeframe"), aStartTime, aEndTime);

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
    mPrefBranch->GetBoolPref(PREF_FORMFILL_SAVE_HTTPS_FORMS, &gSaveHttpsForms);
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

  if (!SaveHttpsForms()) {
    PRBool isHttps = PR_FALSE;
    aActionURL->SchemeIs("https", &isHttps);
    if (isHttps)
      return NS_OK;

    nsresult rv;
    nsCOMPtr<nsIContent> formCont = do_QueryInterface(formElt, &rv);
    NS_ENSURE_SUCCESS(rv, NS_OK);
    nsCOMPtr<nsIDocument> doc;
    doc = formCont->GetOwnerDoc();
    NS_ENSURE_TRUE(doc, NS_OK);
    nsIURI *docURI = doc->GetDocumentURI();
    NS_ENSURE_TRUE(docURI, NS_OK);
    docURI->SchemeIs("https", &isHttps);
    if (isHttps)
      return NS_OK;
  }

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

          
          if (IsValidCCNumber(value))
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



bool
nsFormHistory::IsValidCCNumber(const nsAString &aString)
{
  nsAutoString ccNumber(aString);
  ccNumber.StripChars("-");
  ccNumber.StripWhitespace();
  
  PRUint32 length = ccNumber.Length();
  if (length != 9 && length != 15 && length != 16)
    return false;
  
  PRUint32 total = 0;
  for (PRUint32 i = 0; i < length; i++) {
    PRUnichar ch = ccNumber[length - i - 1];
    if (ch < '0' || ch > '9')
      return false;
    ch -= '0';
    if (i % 2 == 0)
      total += ch;
    else
      total += (ch * 2 / 10) + (ch * 2 % 10);
  }
  return total % 10 == 0;
}

nsresult
nsFormHistory::ExpireOldEntries()
{
  
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 expireDays;
  rv = prefBranch->GetIntPref("browser.formfill.expire_days", &expireDays);
  if (NS_FAILED(rv))
    expireDays = DEFAULT_EXPIRE_DAYS;
  PRInt64 expireTime = PR_Now() - expireDays * 24 * PR_HOURS;

  SendNotification(NS_LITERAL_STRING("before-expireOldEntries"), expireTime);

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

  SendNotification(NS_LITERAL_STRING("expireOldEntries"), expireTime);

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
           "firstUsed INTEGER, lastUsed INTEGER, guid TEXT)"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE INDEX moz_formhistory_index ON moz_formhistory (fieldname)"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE INDEX moz_formhistory_lastused_index ON moz_formhistory (lastUsed)"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE INDEX moz_formhistory_guid_index ON moz_formhistory (guid)"));
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
         "SELECT id, guid FROM moz_formhistory WHERE fieldname=?1 AND value=?2"),
         getter_AddRefs(mDBFindEntry));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
         "SELECT * FROM moz_formhistory WHERE fieldname=?1"),
         getter_AddRefs(mDBFindEntryByName));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "INSERT INTO moz_formhistory (fieldname, value, timesUsed, "
        "firstUsed, lastUsed, guid) VALUES (?1, ?2, ?3, ?4, ?5, ?6)"),
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
      
    case 2:
      rv = MigrateToVersion3();
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
nsFormHistory::MigrateToVersion3()
{
  nsresult rv;

  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
         "SELECT guid FROM moz_formhistory"),
         getter_AddRefs(stmt));

  PRBool columnExists = !!NS_SUCCEEDED(rv);

  if (!columnExists) {
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_formhistory ADD COLUMN guid TEXT"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS moz_formhistory_guid_index ON moz_formhistory (guid)"));
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsCOMPtr<mozIStorageStatement> selectStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
         "SELECT id FROM moz_formhistory WHERE guid isnull"),
         getter_AddRefs(selectStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> updateStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
         "UPDATE moz_formhistory SET guid = ?1 WHERE id = ?2"),
         getter_AddRefs(updateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  while (NS_SUCCEEDED(selectStatement->ExecuteStep(&hasMore)) && hasMore) {
    PRUint64 id = selectStatement->AsInt64(0);

    nsCAutoString guid;
    rv = GenerateGUID(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = updateStatement->BindInt64Parameter(1, id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStatement->BindUTF8StringParameter(0, guid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStatement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mDBConn->SetSchemaVersion(3);
  NS_ENSURE_SUCCESS(rv, rv);

  return transaction.Commit();
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
                  "SELECT fieldname, value, timesUsed, firstUsed, lastUsed, guid "
                  "FROM moz_formhistory"), getter_AddRefs(stmt));
  return NS_SUCCEEDED(rv) ? PR_TRUE : PR_FALSE;
}




nsresult
nsFormHistory::SendNotification(const nsAString &aChangeType, nsISupports *aData)
{
  return mObserverService->NotifyObservers(aData,
                                           "satchel-storage-changed",
                                           PromiseFlatString(aChangeType).get());
}




nsresult
nsFormHistory::SendNotification(const nsAString &aChangeType,
                                const nsAString &aName)
{
  nsresult rv;

  nsCOMPtr<nsISupportsString> fieldName = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  if (!fieldName)
    return NS_ERROR_OUT_OF_MEMORY;

  fieldName->SetData(aName);

  rv = SendNotification(aChangeType, fieldName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




nsresult
nsFormHistory::SendNotification(const nsAString &aChangeType,
                                const nsAString &aName,
                                const nsAString &aValue,
                                const nsAutoString &aGuid)
{
  nsresult rv;

  nsCOMPtr<nsISupportsString> fieldName = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  if (!fieldName)
    return NS_ERROR_OUT_OF_MEMORY;

  fieldName->SetData(aName);

  nsCOMPtr<nsISupportsString> fieldValue = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  if (!fieldValue)
    return NS_ERROR_OUT_OF_MEMORY;

  fieldValue->SetData(aValue);

  nsCOMPtr<nsISupportsString> guid = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
  if (!guid)
    return NS_ERROR_OUT_OF_MEMORY;
  guid->SetData(aGuid);


  nsCOMPtr<nsIMutableArray> notifyData = do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (!notifyData)
    return rv;
  rv = notifyData->AppendElement(fieldName, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = notifyData->AppendElement(fieldValue, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = notifyData->AppendElement(guid, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SendNotification(aChangeType, notifyData);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




nsresult
nsFormHistory::SendNotification(const nsAString &aChangeType,
                                const PRInt64 &aNumber)
{
  nsresult rv;

  nsCOMPtr<nsISupportsPRInt64> valOne = do_CreateInstance(NS_SUPPORTS_PRINT64_CONTRACTID);
  if (!valOne)
    return NS_ERROR_OUT_OF_MEMORY;

  valOne->SetData(aNumber);

  rv = SendNotification(aChangeType, valOne);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




nsresult
nsFormHistory::SendNotification(const nsAString &aChangeType,
                                const PRInt64 &aOne,
                                const PRInt64 &aTwo)
{
  nsresult rv;

  nsCOMPtr<nsISupportsPRInt64> valOne = do_CreateInstance(NS_SUPPORTS_PRINT64_CONTRACTID);
  if (!valOne)
    return NS_ERROR_OUT_OF_MEMORY;

  valOne->SetData(aOne);

  nsCOMPtr<nsISupportsPRInt64> valTwo = do_CreateInstance(NS_SUPPORTS_PRINT64_CONTRACTID);
  if (!valTwo)
    return NS_ERROR_OUT_OF_MEMORY;

  valTwo->SetData(aTwo);

  nsCOMPtr<nsIMutableArray> notifyData = do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (!notifyData)
    return rv;
  rv = notifyData->AppendElement(valOne, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = notifyData->AppendElement(valTwo, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SendNotification(aChangeType, notifyData);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
