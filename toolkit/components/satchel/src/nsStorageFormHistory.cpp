









































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
#include "nsVoidArray.h"
#include "nsCOMArray.h"
#include "mozStorageHelper.h"
#include "mozStorageCID.h"
#include "nsIAutoCompleteSimpleResult.h"
#include "nsTArray.h"
#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"









#define DATABASE_CACHE_PAGES 4000

#define DB_SCHEMA_VERSION   1
#define DB_FILENAME         NS_LITERAL_STRING("formhistory.sqlite")
#define DB_CORRUPT_FILENAME NS_LITERAL_STRING("formhistory.sqlite.corrupt")

#define PR_HOURS (60 * 60 * 1000000)



class nsFormHistoryResult : public nsIAutoCompleteSimpleResult
{
public:
  nsFormHistoryResult(const nsAString &aFieldName)
    : mFieldName(aFieldName) {}

  nsresult Init();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD GetSearchString(nsAString &_result)
  { return mResult->GetSearchString(_result); }
  NS_IMETHOD GetSearchResult(PRUint16 *_result)
  { return mResult->GetSearchResult(_result); }
  NS_IMETHOD GetDefaultIndex(PRInt32 *_result)
  { return mResult->GetDefaultIndex(_result); }
  NS_IMETHOD GetErrorDescription(nsAString &_result)
  { return mResult->GetErrorDescription(_result); }
  NS_IMETHOD GetMatchCount(PRUint32 *_result)
  { return mResult->GetMatchCount(_result); }
  NS_IMETHOD GetValueAt(PRInt32 aIndex, nsAString &_result)
  { return mResult->GetValueAt(aIndex, _result); }
  NS_IMETHOD GetCommentAt(PRInt32 aIndex, nsAString &_result)
  { return mResult->GetCommentAt(aIndex, _result); }
  NS_IMETHOD GetStyleAt(PRInt32 aIndex, nsAString &_result)
  { return mResult->GetStyleAt(aIndex, _result); }
  NS_IMETHOD GetImageAt(PRInt32 aIndex, nsAString &_result)
  { return mResult->GetImageAt(aIndex, _result); }
  NS_IMETHOD RemoveValueAt(PRInt32 aRowIndex, PRBool aRemoveFromDB);
  NS_FORWARD_NSIAUTOCOMPLETESIMPLERESULT(mResult->)

protected:
  nsCOMPtr<nsIAutoCompleteSimpleResult> mResult;
  nsString mFieldName;
};

NS_IMPL_ISUPPORTS2(nsFormHistoryResult,
                   nsIAutoCompleteResult, nsIAutoCompleteSimpleResult)

nsresult
nsFormHistoryResult::Init()
{
  nsresult rv;
  mResult = do_CreateInstance(NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID, &rv);
  return rv;
}

NS_IMETHODIMP
nsFormHistoryResult::RemoveValueAt(PRInt32 aRowIndex, PRBool aRemoveFromDB)
{
  if (!aRemoveFromDB) {
    return mResult->RemoveValueAt(aRowIndex, aRemoveFromDB);
  }

  nsAutoString value;
  nsresult rv = mResult->GetValueAt(aRowIndex, value);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mResult->RemoveValueAt(aRowIndex, aRemoveFromDB);
  NS_ENSURE_SUCCESS(rv, rv);

  nsFormHistory* fh = nsFormHistory::GetInstance();
  NS_ENSURE_TRUE(fh, NS_ERROR_OUT_OF_MEMORY);
  return fh->RemoveEntry(mFieldName, value);
}

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
  PRBool doImport = PR_FALSE;

  nsresult rv = OpenDatabase(&doImport);
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
  if (service)
    service->AddObserver(this, NS_EARLYFORMSUBMIT_SUBJECT, PR_TRUE);

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
  
  PRBool inPrivateBrowsing = PR_FALSE;
  nsCOMPtr<nsIPrivateBrowsingService> pbs =
    do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (pbs)
    pbs->GetPrivateBrowsingEnabled(&inPrivateBrowsing);
  if (inPrivateBrowsing)
    return NS_OK;

  if (!FormHistoryEnabled())
    return NS_OK;

  nsresult rv;
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
        if (!value.IsEmpty()) {
          nsAutoString name;
          inputElt->GetName(name);
          if (name.IsEmpty())
            inputElt->GetId(name);
          if (!name.IsEmpty())
            AddEntry(name, value);
        }
      }
    }
  }

  return transaction.Commit();
}

nsresult
nsFormHistory::CreateTable()
{
  nsresult rv;
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
         "CREATE TABLE moz_formhistory ("
           "id INTEGER PRIMARY KEY, fieldname TEXT NOT NULL, "
           "value TEXT NOT NULL, timesUsed INTEGER NOT NULL, "
           "firstUsed INTEGER NOT NULL, lastUsed INTEGER NOT NULL)"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE INDEX moz_formhistory_index ON moz_formhistory (fieldname)"));
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
        "SELECT value FROM moz_formhistory WHERE fieldname=?1 ORDER BY value ASC"),
        getter_AddRefs(mDBGetMatchingField));
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
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = formHistoryFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mStorageService->OpenDatabase(formHistoryFile, getter_AddRefs(mDBConn));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  PRBool exists;
  mDBConn->TableExists(NS_LITERAL_CSTRING("moz_formhistory"), &exists);
  if (!exists) {
    *aDoImport = PR_TRUE;
    rv = CreateTable();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt32 schemaVersion;
  rv = mDBConn->GetSchemaVersion(&schemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  

  switch (schemaVersion) {
  case 0:
    {
      mozStorageTransaction stepTransaction(mDBConn, PR_FALSE);

      
      NS_WARNING("Could not get formhistory database's schema version!");

      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_formhistory ADD COLUMN timesUsed INTEGER"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_formhistory ADD COLUMN firstUsed INTEGER"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_formhistory ADD COLUMN lastUsed INTEGER"));
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      
      
      nsCOMPtr<mozIStorageStatement> addDefaultValues;
      rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "UPDATE moz_formhistory "
        "SET timesUsed=1, firstUsed=?1, lastUsed=?1"),
        getter_AddRefs(addDefaultValues));
      rv = addDefaultValues->BindInt64Parameter(0, PR_Now() - 24 * PR_HOURS);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = addDefaultValues->Execute();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDBConn->SetSchemaVersion(DB_SCHEMA_VERSION);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = stepTransaction.Commit();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    

  
#ifndef DEBUG
  case DB_SCHEMA_VERSION:
#endif
    break;

  
  
  default:
    {
      
      nsCOMPtr<mozIStorageStatement> stmt;
      rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT fieldname, value, timesUsed, firstUsed, lastUsed "
        "FROM moz_formhistory"), getter_AddRefs(stmt));
      if (NS_SUCCEEDED(rv))
        break;

      
      nsCOMPtr<mozIStorageService> storage =
        do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
      NS_ENSURE_TRUE(storage, NS_ERROR_NOT_AVAILABLE);

      nsCOMPtr<nsIFile> backupFile;
      rv = storage->BackupDatabaseFile(formHistoryFile, DB_CORRUPT_FILENAME,
                                       nsnull, getter_AddRefs(backupFile));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP TABLE moz_formhistory"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = CreateTable();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;
  }
  
  
  transaction.Commit();

  
  StartCache();

  rv = CreateStatements();
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
nsFormHistory::StartCache()
{
  
  if (mDummyStatement)
    return NS_OK;

  
  nsCOMPtr<nsIFile> formHistoryFile;
  nsresult rv = GetDatabaseFile(getter_AddRefs(formHistoryFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mStorageService->OpenDatabase(formHistoryFile,
                                     getter_AddRefs(mDummyConnection));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool tableExists;
  rv = mDummyConnection->TableExists(NS_LITERAL_CSTRING("moz_dummy_table"), &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! tableExists) {
    rv = mDummyConnection->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE TABLE moz_dummy_table (id INTEGER PRIMARY KEY)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  rv = mDummyConnection->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("INSERT OR IGNORE INTO moz_dummy_table VALUES (1)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDummyConnection->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id FROM moz_dummy_table LIMIT 1"),
    getter_AddRefs(mDummyStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool dummyHasResults;
  rv = mDummyStatement->ExecuteStep(&dummyHasResults);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString cacheSizePragma("PRAGMA cache_size=");
  cacheSizePragma.AppendInt(DATABASE_CACHE_PAGES);
  rv = mDummyConnection->ExecuteSimpleSQL(cacheSizePragma);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}








nsresult
nsFormHistory::StopCache()
{
  
  if (! mDummyStatement)
    return NS_OK;

  nsresult rv = mDummyStatement->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  mDummyStatement = nsnull;
  return NS_OK;
}


nsresult
nsFormHistory::AutoCompleteSearch(const nsAString &aInputName,
                                  const nsAString &aInputValue,
                                  nsIAutoCompleteSimpleResult *aPrevResult,
                                  nsIAutoCompleteResult **aResult)
{
  if (!FormHistoryEnabled())
    return NS_OK;

  nsCOMPtr<nsIAutoCompleteSimpleResult> result;

  if (aPrevResult) {
    result = aPrevResult;

    PRUint32 matchCount;
    result->GetMatchCount(&matchCount);

    for (PRInt32 i = matchCount - 1; i >= 0; --i) {
      nsAutoString match;
      result->GetValueAt(i, match);
      if (!StringBeginsWith(match, aInputValue,
                            nsCaseInsensitiveStringComparator())) {
        result->RemoveValueAt(i, PR_FALSE);
      }
    }
  } else {
    nsCOMPtr<nsFormHistoryResult> fhResult =
      new nsFormHistoryResult(aInputName);
    NS_ENSURE_TRUE(fhResult, NS_ERROR_OUT_OF_MEMORY);
    nsresult rv = fhResult->Init();
    NS_ENSURE_SUCCESS(rv, rv);
    reinterpret_cast<nsCOMPtr<nsIAutoCompleteSimpleResult>*>(&fhResult)->swap(result);

    result->SetSearchString(aInputValue);

    
    mozStorageStatementScoper scope(mDBGetMatchingField);
    rv = mDBGetMatchingField->BindStringParameter(0, aInputName);
    NS_ENSURE_SUCCESS(rv,rv);

    PRBool hasMore = PR_FALSE;
    PRUint32 count = 0;
    while (NS_SUCCEEDED(mDBGetMatchingField->ExecuteStep(&hasMore)) &&
           hasMore) {
      nsAutoString entryString;
      mDBGetMatchingField->GetString(0, entryString);
      
      if(StringBeginsWith(entryString, aInputValue,
                          nsCaseInsensitiveStringComparator())) {
        result->AppendMatch(entryString, EmptyString(), EmptyString(), EmptyString());
        ++count;
      }
    }
    if (count > 0) {
      result->SetSearchResult(nsIAutoCompleteResult::RESULT_SUCCESS);
      result->SetDefaultIndex(0);
    } else {
      result->SetSearchResult(nsIAutoCompleteResult::RESULT_NOMATCH);
      result->SetDefaultIndex(-1);
    }
  }

  *aResult = result;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
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
