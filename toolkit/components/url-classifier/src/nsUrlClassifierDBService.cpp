






































#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozStorageCID.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsAutoLock.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIDirectoryService.h"
#include "nsIObserverService.h"
#include "nsIProperties.h"
#include "nsIProxyObjectManager.h"
#include "nsToolkitCompsCID.h"
#include "nsUrlClassifierDBService.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsXPCOMStrings.h"
#include "prlog.h"
#include "prprf.h"


#if defined(PR_LOGGING)
static const PRLogModuleInfo *gUrlClassifierDbServiceLog = nsnull;
#define LOG(args) PR_LOG(gUrlClassifierDbServiceLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif


#define DATABASE_FILENAME "urlclassifier2.sqlite"


static nsUrlClassifierDBService* sUrlClassifierDBService;


static nsIThread* gDbBackgroundThread = nsnull;



static PRBool gShuttingDownThread = PR_FALSE;

static const char* kNEW_TABLE_SUFFIX = "_new";



static const unsigned char kRot13Table[256] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 62, 63, 64, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 91, 92, 93, 94, 95, 96,
  110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 97, 98,
  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 123, 124, 125, 126,
  127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
  142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156,
  157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171,
  172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
  187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201,
  202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216,
  217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231,
  232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246,
  247, 248, 249, 250, 251, 252, 253, 254, 255 };


static void
Rot13Line(nsCString &line)
{
  nsCString::iterator start, end;
  line.BeginWriting(start);
  line.EndWriting(end);
  while (start != end) {
    *start = kRot13Table[static_cast<PRInt32>(*start)];
    ++start;
  }
}




class nsUrlClassifierDBServiceWorker : public nsIUrlClassifierDBServiceWorker
{
public:
  nsUrlClassifierDBServiceWorker();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERDBSERVICE
  NS_DECL_NSIURLCLASSIFIERDBSERVICEWORKER

private:
  
  ~nsUrlClassifierDBServiceWorker();

  
  nsUrlClassifierDBServiceWorker(nsUrlClassifierDBServiceWorker&);

  
  
  void GetDbTableName(const nsACString& aTableName, nsCString* aDbTableName);

  
  nsresult OpenDb();

  
  nsresult MaybeCreateTable(const nsCString& aTableName);

  
  nsresult MaybeDropTable(const nsCString& aTableName);

  
  
  nsresult MaybeSwapTables(const nsCString& aVersionLine);

  
  
  
  nsresult ParseVersionString(const nsCSubstring& aLine,
                              nsCString* aTableName,
                              PRBool* aIsUpdate);

  
  
  
  nsresult ProcessNewTable(const nsCSubstring& aLine,
                           nsCString* aTableName,
                           mozIStorageStatement** aUpdateStatement,
                           mozIStorageStatement** aDeleteStatement);

  
  
  nsresult ProcessUpdateTable(const nsCSubstring& aLine,
                              const nsCString& aTableName,
                              mozIStorageStatement* aUpdateStatement,
                              mozIStorageStatement* aDeleteStatement);

  
  
  
  mozIStorageConnection* mConnection;

  
  PRBool mHasPendingUpdate;

  
  
  
  nsTArray<nsCString> mTableUpdateLines;

  
  
  nsCString mPendingStreamUpdate;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUrlClassifierDBServiceWorker,
                              nsIUrlClassifierDBServiceWorker)

nsUrlClassifierDBServiceWorker::nsUrlClassifierDBServiceWorker()
  : mConnection(nsnull), mHasPendingUpdate(PR_FALSE), mTableUpdateLines()
{
}
nsUrlClassifierDBServiceWorker::~nsUrlClassifierDBServiceWorker()
{
  NS_ASSERTION(mConnection == nsnull,
               "Db connection not closed, leaking memory!  Call CloseDb "
               "to close the connection.");
}



NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::Exists(const nsACString& tableName,
                                       const nsACString& key,
                                       nsIUrlClassifierCallback *c)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  nsCAutoString dbTableName;
  GetDbTableName(tableName, &dbTableName);

  nsCOMPtr<mozIStorageStatement> selectStatement;
  nsCAutoString statement;
  statement.AssignLiteral("SELECT value FROM ");
  statement.Append(dbTableName);
  statement.AppendLiteral(" WHERE key = ?1");

  rv = mConnection->CreateStatement(statement,
                                    getter_AddRefs(selectStatement));

  nsAutoString value;
  
  
  if (NS_SUCCEEDED(rv)) {
    nsCString keyROT13(key);
    Rot13Line(keyROT13);
    rv = selectStatement->BindUTF8StringParameter(0, keyROT13);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore = PR_FALSE;
    rv = selectStatement->ExecuteStep(&hasMore);
    
    if (NS_SUCCEEDED(rv) && hasMore) {
      selectStatement->GetString(0, value);
    }
  }

  c->HandleEvent(NS_ConvertUTF16toUTF8(value));
  return NS_OK;
}



NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CheckTables(const nsACString & tableNames,
                                            nsIUrlClassifierCallback *c)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  nsCAutoString changedTables;

  
  
  PRUint32 cur = 0;
  PRInt32 next;
  while (cur < tableNames.Length()) {
    next = tableNames.FindChar(',', cur);
    if (kNotFound == next) {
      next = tableNames.Length();
    }
    const nsCSubstring &tableName = Substring(tableNames, cur, next - cur);
    cur = next + 1;

    nsCString dbTableName;
    GetDbTableName(tableName, &dbTableName);
    PRBool exists;
    nsresult rv = mConnection->TableExists(dbTableName, &exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!exists) {
      if (changedTables.Length() > 0)
        changedTables.Append(",");
      changedTables.Append(tableName);
    }
  }

  c->HandleEvent(changedTables);
  return NS_OK;
}



NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::UpdateTables(const nsACString& updateString,
                                             nsIUrlClassifierCallback *c)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  LOG(("Updating tables\n"));

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  rv = mConnection->BeginTransaction();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to begin transaction");

  
  PRUint32 cur = 0;
  PRInt32 next;
  PRInt32 count = 0;
  nsCAutoString dbTableName;
  nsCAutoString lastTableLine;
  nsCOMPtr<mozIStorageStatement> updateStatement;
  nsCOMPtr<mozIStorageStatement> deleteStatement;
  while(cur < updateString.Length() &&
        (next = updateString.FindChar('\n', cur)) != kNotFound) {
    const nsCSubstring &line = Substring(updateString, cur, next - cur);
    cur = next + 1; 

    
    if (line.Length() == 0)
      continue;

    count++;

    if ('[' == line[0]) {
      rv = ProcessNewTable(line, &dbTableName,
                           getter_AddRefs(updateStatement),
                           getter_AddRefs(deleteStatement));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "malformed table line");
      if (NS_SUCCEEDED(rv)) {
        
        
        if (lastTableLine.Length() > 0) {
          
          rv = MaybeSwapTables(lastTableLine);
          if (NS_SUCCEEDED(rv)) {
            mConnection->CommitTransaction();
            c->HandleEvent(lastTableLine);
          } else {
            
            mConnection->RollbackTransaction();
          }
          mConnection->BeginTransaction();
        }
        lastTableLine.Assign(line);
      }
    } else {
      rv = ProcessUpdateTable(line, dbTableName, updateStatement,
                              deleteStatement);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "malformed update line");
    }
  }
  LOG(("Num update lines: %d\n", count));

  rv = MaybeSwapTables(lastTableLine);
  if (NS_SUCCEEDED(rv)) {
    mConnection->CommitTransaction();
    c->HandleEvent(lastTableLine);
  } else {
    
    mConnection->RollbackTransaction();
  }

  LOG(("Finishing table update\n"));
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::Update(const nsACString& chunk)
{
  LOG(("Update from Stream."));
  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  nsCAutoString updateString(mPendingStreamUpdate);
  updateString.Append(chunk);
  
  nsCOMPtr<mozIStorageStatement> updateStatement;
  nsCOMPtr<mozIStorageStatement> deleteStatement;
  nsCAutoString dbTableName;

  
  
  if (!mHasPendingUpdate) {
    mConnection->BeginTransaction();
    mHasPendingUpdate = PR_TRUE;
  } else {
    PRUint32 numTables = mTableUpdateLines.Length();
    if (numTables > 0) {
      const nsCSubstring &line = Substring(
              mTableUpdateLines[numTables - 1], 0);

      rv = ProcessNewTable(line, &dbTableName,
                           getter_AddRefs(updateStatement),
                           getter_AddRefs(deleteStatement));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "malformed table line");
    }
  }

  PRUint32 cur = 0;
  PRInt32 next;
  while(cur < updateString.Length() &&
        (next = updateString.FindChar('\n', cur)) != kNotFound) {
    const nsCSubstring &line = Substring(updateString, cur, next - cur);
    cur = next + 1; 

    
    if (line.Length() == 0)
      continue;

    if ('[' == line[0]) {
      rv = ProcessNewTable(line, &dbTableName,
                           getter_AddRefs(updateStatement),
                           getter_AddRefs(deleteStatement));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "malformed table line");
      if (NS_SUCCEEDED(rv)) {
        
        mTableUpdateLines.AppendElement(line);
      }
    } else {
      rv = ProcessUpdateTable(line, dbTableName, updateStatement,
                              deleteStatement);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "malformed update line");
    }
  }
  
  mPendingStreamUpdate = Substring(updateString, cur);
  LOG(("pending stream update: %s", mPendingStreamUpdate.get()));

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::Finish(nsIUrlClassifierCallback *c)
{
  if (!mHasPendingUpdate)
    return NS_OK;

  if (gShuttingDownThread) {
    mConnection->RollbackTransaction();
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv = NS_OK;
  for (PRUint32 i = 0; i < mTableUpdateLines.Length(); ++i) {
    rv = MaybeSwapTables(mTableUpdateLines[i]);
    if (NS_FAILED(rv)) {
      break;
    }
  }
  
  if (NS_SUCCEEDED(rv)) {
    LOG(("Finish, committing transaction"));
    mConnection->CommitTransaction();

    
    for (PRUint32 i = 0; i < mTableUpdateLines.Length(); ++i) {
      c->HandleEvent(mTableUpdateLines[i]);
    }
  } else {
    LOG(("Finish failed (swap table error?), rolling back transaction"));
    mConnection->RollbackTransaction();
  }

  mTableUpdateLines.Clear();
  mPendingStreamUpdate.Truncate();
  mHasPendingUpdate = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CancelStream()
{
  if (!mHasPendingUpdate)
    return NS_OK;

  LOG(("CancelStream, rolling back transaction"));
  mConnection->RollbackTransaction();

  mTableUpdateLines.Clear();
  mPendingStreamUpdate.Truncate();
  mHasPendingUpdate = PR_FALSE;
  
  return NS_OK;
}





NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CloseDb()
{
  if (mConnection != nsnull) {
    NS_RELEASE(mConnection);
    LOG(("urlclassifier db closed\n"));
  }
  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ProcessNewTable(
                                    const nsCSubstring& aLine,
                                    nsCString* aDbTableName,
                                    mozIStorageStatement** aUpdateStatement,
                                    mozIStorageStatement** aDeleteStatement)
{
  
  
  
  PRBool isUpdate = PR_FALSE;

  
  nsresult rv = ParseVersionString(aLine, aDbTableName, &isUpdate);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  
  if (!isUpdate)
    aDbTableName->Append(kNEW_TABLE_SUFFIX);

  
  rv = MaybeCreateTable(*aDbTableName);
  if (NS_FAILED(rv))
    return rv;

  
  nsCAutoString statement;
  statement.AssignLiteral("INSERT OR REPLACE INTO ");
  statement.Append(*aDbTableName);
  statement.AppendLiteral(" VALUES (?1, ?2)");
  rv = mConnection->CreateStatement(statement, aUpdateStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  
  statement.AssignLiteral("DELETE FROM ");
  statement.Append(*aDbTableName);
  statement.AppendLiteral(" WHERE key = ?1");
  rv = mConnection->CreateStatement(statement, aDeleteStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ProcessUpdateTable(
                                   const nsCSubstring& aLine,
                                   const nsCString& aTableName,
                                   mozIStorageStatement* aUpdateStatement,
                                   mozIStorageStatement* aDeleteStatement)
{
  
  if (aTableName.Length() == 0)
    return NS_ERROR_FAILURE;

  if (!aUpdateStatement || !aDeleteStatement) {
    NS_NOTREACHED("Statements NULL but table is not");
    return NS_ERROR_FAILURE;
  }
  
  if (aLine.Length() < 2)
    return NS_ERROR_FAILURE;

  char op = aLine[0];
  PRInt32 spacePos = aLine.FindChar('\t');
  nsresult rv = NS_ERROR_FAILURE;

  if ('+' == op && spacePos != kNotFound) {
    
    const nsCSubstring &key = Substring(aLine, 1, spacePos - 1);
    const nsCSubstring &value = Substring(aLine, spacePos + 1);

    
    
    nsCString keyROT13(key);
    Rot13Line(keyROT13);
    
    aUpdateStatement->BindUTF8StringParameter(0, keyROT13);
    aUpdateStatement->BindUTF8StringParameter(1, value);

    rv = aUpdateStatement->Execute();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to update");
  } else if ('-' == op) {
    
    nsCString keyROT13;
    if (spacePos == kNotFound) {
      
      const nsCSubstring &key = Substring(aLine, 1);
      keyROT13.Assign(key);
    } else {
      
      const nsCSubstring &key = Substring(aLine, 1, spacePos - 1);
      keyROT13.Assign(key);
    }
    Rot13Line(keyROT13);
    aDeleteStatement->BindUTF8StringParameter(0, keyROT13);

    rv = aDeleteStatement->Execute();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to delete");
  }

  return rv;
}

nsresult
nsUrlClassifierDBServiceWorker::OpenDb()
{
  
  if (mConnection != nsnull)
    return NS_OK;

  LOG(("Opening db\n"));
  
  nsCOMPtr<nsIFile> dbFile;

  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(dbFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = dbFile->Append(NS_LITERAL_STRING(DATABASE_FILENAME));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageService> storageService =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = storageService->OpenDatabase(dbFile, &mConnection);
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = dbFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = storageService->OpenDatabase(dbFile, &mConnection);
  }
  return rv;
}

nsresult
nsUrlClassifierDBServiceWorker::MaybeCreateTable(const nsCString& aTableName)
{
  LOG(("MaybeCreateTable %s\n", aTableName.get()));

  nsCOMPtr<mozIStorageStatement> createStatement;
  nsCString statement;
  statement.Assign("CREATE TABLE IF NOT EXISTS ");
  statement.Append(aTableName);
  statement.Append(" (key TEXT PRIMARY KEY, value TEXT)");
  nsresult rv = mConnection->CreateStatement(statement,
                                             getter_AddRefs(createStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  return createStatement->Execute();
}

nsresult
nsUrlClassifierDBServiceWorker::MaybeDropTable(const nsCString& aTableName)
{
  LOG(("MaybeDropTable %s\n", aTableName.get()));
  nsCAutoString statement("DROP TABLE IF EXISTS ");
  statement.Append(aTableName);
  return mConnection->ExecuteSimpleSQL(statement);
}

nsresult
nsUrlClassifierDBServiceWorker::MaybeSwapTables(const nsCString& aVersionLine)
{
  if (aVersionLine.Length() == 0)
    return NS_ERROR_FAILURE;

  
  nsCAutoString tableName;
  PRBool isUpdate;
  nsresult rv = ParseVersionString(aVersionLine, &tableName, &isUpdate);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (isUpdate)
    return NS_OK;

  
  
  rv = MaybeDropTable(tableName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString newTableName(tableName);
  newTableName.Append(kNEW_TABLE_SUFFIX);

  
  nsCAutoString sql("ALTER TABLE ");
  sql.Append(newTableName);
  sql.Append(" RENAME TO ");
  sql.Append(tableName);
  rv = mConnection->ExecuteSimpleSQL(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("tables swapped (%s)\n", tableName.get()));

  return NS_OK;
}


nsresult
nsUrlClassifierDBServiceWorker::ParseVersionString(const nsCSubstring& aLine,
                                                   nsCString* aTableName,
                                                   PRBool* aIsUpdate)
{
  
  if (aLine.Length() == 0)
    return NS_ERROR_FAILURE;

  
  const PRUint32 MAX_LENGTH = 2048;
  if (aLine.Length() > MAX_LENGTH)
    return NS_ERROR_FAILURE;

  nsCAutoString lineData(aLine);
  char tableNameBuf[MAX_LENGTH], endChar = ' ';
  PRInt32 majorVersion, minorVersion, numConverted;
  
  numConverted = PR_sscanf(lineData.get(), "[%s %d.%d update%c", tableNameBuf,
                           &majorVersion, &minorVersion, &endChar);
  if (numConverted != 4 || endChar != ']') {
    
    numConverted = PR_sscanf(lineData.get(), "[%s %d.%d%c", tableNameBuf,
                             &majorVersion, &minorVersion, &endChar);
    if (numConverted != 4 || endChar != ']')
      return NS_ERROR_FAILURE;
    *aIsUpdate = PR_FALSE;
  } else {
    
    *aIsUpdate = PR_TRUE;
  }

  LOG(("Is update? %d\n", *aIsUpdate));

  
  
  GetDbTableName(nsCAutoString(tableNameBuf), aTableName);
  return NS_OK;
}

void
nsUrlClassifierDBServiceWorker::GetDbTableName(const nsACString& aTableName,
                                               nsCString* aDbTableName)
{
  aDbTableName->Assign(aTableName);
  aDbTableName->ReplaceChar('-', '_');
}




NS_IMPL_THREADSAFE_ISUPPORTS2(nsUrlClassifierDBService,
                              nsIUrlClassifierDBService,
                              nsIObserver)

 nsUrlClassifierDBService*
nsUrlClassifierDBService::GetInstance()
{
  if (!sUrlClassifierDBService) {
    sUrlClassifierDBService = new nsUrlClassifierDBService();
    if (!sUrlClassifierDBService)
      return nsnull;

    NS_ADDREF(sUrlClassifierDBService);   

    if (NS_FAILED(sUrlClassifierDBService->Init())) {
      NS_RELEASE(sUrlClassifierDBService);
      return nsnull;
    }
  } else {
    
    NS_ADDREF(sUrlClassifierDBService);   
  }
  return sUrlClassifierDBService;
}


nsUrlClassifierDBService::nsUrlClassifierDBService()
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
  nsCOMPtr<mozIStorageService> storageService =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = NS_NewThread(&gDbBackgroundThread);
  if (NS_FAILED(rv))
    return rv;

  mWorker = new nsUrlClassifierDBServiceWorker();
  if (!mWorker)
    return NS_ERROR_OUT_OF_MEMORY;

  
  nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1");
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->AddObserver(this, "profile-before-change", PR_FALSE);
  observerService->AddObserver(this, "xpcom-shutdown-threads", PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::Exists(const nsACString& tableName,
                                 const nsACString& key,
                                 nsIUrlClassifierCallback *c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  
  nsCOMPtr<nsIUrlClassifierCallback> proxyCallback;
  rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(nsIUrlClassifierCallback),
                            c,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxyCallback));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->Exists(tableName, key, proxyCallback);
}

NS_IMETHODIMP
nsUrlClassifierDBService::CheckTables(const nsACString & tableNames,
                                      nsIUrlClassifierCallback *c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  
  nsCOMPtr<nsIUrlClassifierCallback> proxyCallback;
  rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(nsIUrlClassifierCallback),
                            c,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxyCallback));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->CheckTables(tableNames, proxyCallback);
}

NS_IMETHODIMP
nsUrlClassifierDBService::UpdateTables(const nsACString& updateString,
                                       nsIUrlClassifierCallback *c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  
  nsCOMPtr<nsIUrlClassifierCallback> proxyCallback;
  rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(nsIUrlClassifierCallback),
                            c,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxyCallback));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->UpdateTables(updateString, proxyCallback);
}

NS_IMETHODIMP
nsUrlClassifierDBService::Update(const nsACString& aUpdateChunk)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->Update(aUpdateChunk);
}

NS_IMETHODIMP
nsUrlClassifierDBService::Finish(nsIUrlClassifierCallback *c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  
  nsCOMPtr<nsIUrlClassifierCallback> proxyCallback;
  rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(nsIUrlClassifierCallback),
                            c,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxyCallback));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->Finish(proxyCallback);
}

NS_IMETHODIMP
nsUrlClassifierDBService::CancelStream()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->CancelStream();
}

NS_IMETHODIMP
nsUrlClassifierDBService::Observe(nsISupports *aSubject, const char *aTopic,
                                  const PRUnichar *aData)
{
  NS_ASSERTION(strcmp(aTopic, "profile-before-change") == 0 ||
               strcmp(aTopic, "xpcom-shutdown-threads") == 0,
               "Unexpected observer topic");

  Shutdown();

  return NS_OK;
}


nsresult
nsUrlClassifierDBService::Shutdown()
{
  LOG(("shutting down db service\n"));

  if (!gDbBackgroundThread)
    return NS_OK;

  nsresult rv;
  
  if (mWorker) {
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
    rv = NS_GetProxyForObject(gDbBackgroundThread,
                              NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                              mWorker,
                              NS_PROXY_ASYNC,
                              getter_AddRefs(proxy));
    if (NS_SUCCEEDED(rv)) {
      rv = proxy->CloseDb();
      NS_ASSERTION(NS_SUCCEEDED(rv), "failed to post close db event");
    }
  }
  LOG(("joining background thread"));

  gShuttingDownThread = PR_TRUE;
  gDbBackgroundThread->Shutdown();
  NS_RELEASE(gDbBackgroundThread);

  return NS_OK;
}
