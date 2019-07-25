





































#include "OpenDatabaseHelper.h"
#include "IDBEvents.h"
#include "IDBFactory.h"
#include "IndexedDatabaseManager.h"

#include "mozilla/storage.h"
#include "nsIFile.h"

#include "nsContentUtils.h"
#include "nsEscape.h"
#include "nsThreadUtils.h"

USING_INDEXEDDB_NAMESPACE

const extern PRUint32 kDefaultDatabaseTimeoutSeconds = 30; 

namespace {

nsresult
GetDatabaseFile(const nsACString& aASCIIOrigin,
                const nsAString& aName,
                nsIFile** aDatabaseFile)
{
  NS_ASSERTION(!aASCIIOrigin.IsEmpty() && !aName.IsEmpty(), "Bad arguments!");

  nsCOMPtr<nsIFile> dbFile;
  nsresult rv = IDBFactory::GetDirectory(getter_AddRefs(dbFile));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertASCIItoUTF16 originSanitized(aASCIIOrigin);
  originSanitized.ReplaceChar(":/", '+');

  rv = dbFile->Append(originSanitized);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString filename;
  filename.AppendInt(HashString(aName));

  nsCString escapedName;
  if (!NS_Escape(NS_ConvertUTF16toUTF8(aName), escapedName, url_XPAlphas)) {
    NS_WARNING("Can't escape database name!");
    return NS_ERROR_UNEXPECTED;
  }

  const char* forwardIter = escapedName.BeginReading();
  const char* backwardIter = escapedName.EndReading() - 1;

  nsCString substring;
  while (forwardIter <= backwardIter && substring.Length() < 21) {
    if (substring.Length() % 2) {
      substring.Append(*backwardIter--);
    }
    else {
      substring.Append(*forwardIter++);
    }
  }

  filename.Append(NS_ConvertASCIItoUTF16(substring));
  filename.AppendLiteral(".sqlite");

  rv = dbFile->Append(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  dbFile.forget(aDatabaseFile);
  return NS_OK;
}

nsresult
CreateTables(mozIStorageConnection* aDBConn)
{
  NS_PRECONDITION(!NS_IsMainThread(),
                  "Creating tables on the main thread!");
  NS_PRECONDITION(aDBConn, "Passing a null database connection!");

  
  nsresult rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE database ("
      "name TEXT NOT NULL, "
      "version INTEGER NOT NULL DEFAULT 0, "
      "dataVersion INTEGER NOT NULL"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_store ("
      "id INTEGER, "
      "name TEXT NOT NULL, "
      "key_path TEXT NOT NULL, "
      "auto_increment INTEGER NOT NULL DEFAULT 0, "
      "PRIMARY KEY (id), "
      "UNIQUE (name)"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_data ("
      "id INTEGER, "
      "object_store_id INTEGER NOT NULL, "
      "data BLOB NOT NULL, "
      "key_value DEFAULT NULL, " 
      "PRIMARY KEY (id), "
      "FOREIGN KEY (object_store_id) REFERENCES object_store(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE UNIQUE INDEX key_index "
    "ON object_data (key_value, object_store_id);"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE ai_object_data ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "object_store_id INTEGER NOT NULL, "
      "data BLOB NOT NULL, "
      "FOREIGN KEY (object_store_id) REFERENCES object_store(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE UNIQUE INDEX ai_key_index "
    "ON ai_object_data (id, object_store_id);"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE object_store_index ("
      "id INTEGER, "
      "object_store_id INTEGER NOT NULL, "
      "name TEXT NOT NULL, "
      "key_path TEXT NOT NULL, "
      "unique_index INTEGER NOT NULL, "
      "object_store_autoincrement INTERGER NOT NULL, "
      "PRIMARY KEY (id), "
      "UNIQUE (object_store_id, name), "
      "FOREIGN KEY (object_store_id) REFERENCES object_store(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE index_data ("
      "id INTEGER, "
      "index_id INTEGER NOT NULL, "
      "object_data_id INTEGER NOT NULL, "
      "object_data_key NOT NULL, " 
      "value NOT NULL, "
      "PRIMARY KEY (id), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE, "
      "FOREIGN KEY (object_data_id) REFERENCES object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX value_index "
    "ON index_data (index_id, value);"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE unique_index_data ("
      "id INTEGER, "
      "index_id INTEGER NOT NULL, "
      "object_data_id INTEGER NOT NULL, "
      "object_data_key NOT NULL, " 
      "value NOT NULL, "
      "PRIMARY KEY (id), "
      "UNIQUE (index_id, value), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE "
      "FOREIGN KEY (object_data_id) REFERENCES object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE ai_index_data ("
      "id INTEGER, "
      "index_id INTEGER NOT NULL, "
      "ai_object_data_id INTEGER NOT NULL, "
      "value NOT NULL, "
      "PRIMARY KEY (id), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE, "
      "FOREIGN KEY (ai_object_data_id) REFERENCES ai_object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX ai_value_index "
    "ON ai_index_data (index_id, value);"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE ai_unique_index_data ("
      "id INTEGER, "
      "index_id INTEGER NOT NULL, "
      "ai_object_data_id INTEGER NOT NULL, "
      "value NOT NULL, "
      "PRIMARY KEY (id), "
      "UNIQUE (index_id, value), "
      "FOREIGN KEY (index_id) REFERENCES object_store_index(id) ON DELETE "
        "CASCADE, "
      "FOREIGN KEY (ai_object_data_id) REFERENCES ai_object_data(id) ON DELETE "
        "CASCADE"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDBConn->SetSchemaVersion(DB_SCHEMA_VERSION);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
CreateMetaData(mozIStorageConnection* aConnection,
               const nsAString& aName)
{
  NS_PRECONDITION(!NS_IsMainThread(), "Wrong thread!");
  NS_PRECONDITION(aConnection, "Null database!");

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT OR REPLACE INTO database (name, dataVersion) "
    "VALUES (:name, :dataVersion)"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("name"), aName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("dataVersion"),
                             JS_STRUCTURED_CLONE_VERSION);
  NS_ENSURE_SUCCESS(rv, rv);

  return stmt->Execute();
}

nsresult
UpgradeSchemaFrom4To5(mozIStorageConnection* aConnection)
{
  nsresult rv;

  mozStorageTransaction transaction(aConnection, false,
                                 mozIStorageConnection::TRANSACTION_IMMEDIATE);

  
  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT name, version, dataVersion "
    "FROM database"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString name;
  PRInt32 intVersion;
  PRInt64 dataVersion;

  {
    mozStorageStatementScoper scoper(stmt);

    bool hasResults;
    rv = stmt->ExecuteStep(&hasResults);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(hasResults, NS_ERROR_FAILURE);

    nsString version;
    rv = stmt->GetString(1, version);
    NS_ENSURE_SUCCESS(rv, rv);

    intVersion = version.ToInteger(&rv, 10);
    if (NS_FAILED(rv)) {
      intVersion = 0;
    }

    rv = stmt->GetString(0, name);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->GetInt64(2, &dataVersion);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TABLE database"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE database ("
      "name TEXT NOT NULL, "
      "version INTEGER NOT NULL DEFAULT 0, "
      "dataVersion INTEGER NOT NULL"
    ");"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO database (name, version, dataVersion) "
    "VALUES (:name, :version, :dataVersion)"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  {
    mozStorageStatementScoper scoper(stmt);

    rv = stmt->BindStringParameter(0, name);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindInt32Parameter(1, intVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->BindInt64Parameter(2, dataVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aConnection->SetSchemaVersion(DB_SCHEMA_VERSION);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
CreateDatabaseConnection(const nsAString& aName,
                         nsIFile* aDBFile,
                         mozIStorageConnection** aConnection)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIFile> dbDirectory;
  nsresult rv = aDBFile->GetParent(getter_AddRefs(dbDirectory));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = aDBFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_CSTRING(quotaVFSName, "quota");

  nsCOMPtr<mozIStorageServiceQuotaManagement> ss =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(ss, NS_ERROR_FAILURE);

  nsCOMPtr<mozIStorageConnection> connection;
  rv = ss->OpenDatabaseWithVFS(aDBFile, quotaVFSName,
                               getter_AddRefs(connection));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = aDBFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);

    exists = false;

    rv = ss->OpenDatabaseWithVFS(aDBFile, quotaVFSName,
                                 getter_AddRefs(connection));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 schemaVersion;
  rv = connection->GetSchemaVersion(&schemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  if (schemaVersion != DB_SCHEMA_VERSION) {
    
    PR_STATIC_ASSERT(DB_SCHEMA_VERSION == 5);
    if (schemaVersion == 4) {
      rv = UpgradeSchemaFrom4To5(connection);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      
      if (exists) {
        
        rv = aDBFile->Remove(false);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = ss->OpenDatabaseWithVFS(aDBFile, quotaVFSName,
                                     getter_AddRefs(connection));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      mozStorageTransaction transaction(connection, false,
                                        mozIStorageConnection::TRANSACTION_IMMEDIATE);
      rv = CreateTables(connection);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = CreateMetaData(connection, aName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = transaction.Commit();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  NS_ASSERTION(NS_SUCCEEDED(connection->GetSchemaVersion(&schemaVersion)) &&
               schemaVersion == DB_SCHEMA_VERSION,
               "CreateTables failed!");

  
  rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "PRAGMA foreign_keys = ON;"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  connection.forget(aConnection);
  return NS_OK;
}

class SetVersionHelper : public AsyncConnectionHelper,
                         public IDBTransactionListener
{
public:
  SetVersionHelper(IDBTransaction* aTransaction,
                   IDBOpenDBRequest* aRequest,
                   OpenDatabaseHelper* aHelper,
                   PRUint64 aRequestedVersion,
                   PRUint64 aCurrentVersion)
  : AsyncConnectionHelper(aTransaction, aRequest),
    mOpenRequest(aRequest), mOpenHelper(aHelper),
    mRequestedVersion(aRequestedVersion),
    mCurrentVersion(aCurrentVersion)
  {
    mTransaction->SetTransactionListener(this);
  }

  NS_DECL_ISUPPORTS_INHERITED

  nsresult GetSuccessResult(JSContext* aCx,
                            jsval* aVal);

  static
  void QueueVersionChange(nsTArray<nsRefPtr<IDBDatabase> >& aDatabases,
                          void* aClosure);
protected:
  nsresult DoDatabaseWork(mozIStorageConnection* aConnection);
  nsresult Init();

  
  
  void OnError() { }

  
  already_AddRefed<nsDOMEvent> CreateSuccessEvent();

  nsresult NotifyTransactionComplete(IDBTransaction* aTransaction);

private:
  
  nsRefPtr<OpenDatabaseHelper> mOpenHelper;
  nsRefPtr<IDBOpenDBRequest> mOpenRequest;
  PRUint64 mRequestedVersion;
  PRUint64 mCurrentVersion;
};




class VersionChangeEventsRunnable : public nsRunnable
{
public:
  VersionChangeEventsRunnable(
                            IDBDatabase* aRequestingDatabase,
                            IDBOpenDBRequest* aRequest,
                            nsTArray<nsRefPtr<IDBDatabase> >& aWaitingDatabases,
                            PRInt64 aOldVersion,
                            PRInt64 aNewVersion)
  : mRequestingDatabase(aRequestingDatabase),
    mRequest(aRequest),
    mOldVersion(aOldVersion),
    mNewVersion(aNewVersion)
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(aRequestingDatabase, "Null pointer!");
    NS_ASSERTION(aRequest, "Null pointer!");

    if (!mWaitingDatabases.SwapElements(aWaitingDatabases)) {
      NS_ERROR("This should never fail!");
    }
  }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    
    
    PRUint32 count = mWaitingDatabases.Length();
    for (PRUint32 index = 0; index < count; index++) {
      nsRefPtr<IDBDatabase>& database = mWaitingDatabases[index];

      if (database->IsClosed()) {
        continue;
      }

      
      nsCOMPtr<nsIDocument> ownerDoc = database->GetOwnerDocument();
      nsIBFCacheEntry* bfCacheEntry;
      if (ownerDoc && (bfCacheEntry = ownerDoc->GetBFCacheEntry())) {
        bfCacheEntry->RemoveFromBFCacheSync();
        NS_ASSERTION(database->IsClosed(),
                     "Kicking doc out of bfcache should have closed database");
        continue;
      }

      
      nsRefPtr<nsDOMEvent> event = 
        IDBVersionChangeEvent::Create(mOldVersion, mNewVersion);
      NS_ENSURE_TRUE(event, NS_ERROR_FAILURE);

      bool dummy;
      database->DispatchEvent(event, &dummy);
    }

    
    
    for (PRUint32 index = 0; index < count; index++) {
      if (!mWaitingDatabases[index]->IsClosed()) {
        nsRefPtr<nsDOMEvent> event =
          IDBVersionChangeEvent::CreateBlocked(mOldVersion, mNewVersion);
        NS_ENSURE_TRUE(event, NS_ERROR_FAILURE);

        bool dummy;
        mRequest->DispatchEvent(event, &dummy);

        break;
      }
    }

    return NS_OK;
  }

private:
  nsRefPtr<IDBDatabase> mRequestingDatabase;
  nsRefPtr<IDBOpenDBRequest> mRequest;
  nsTArray<nsRefPtr<IDBDatabase> > mWaitingDatabases;
  PRInt64 mOldVersion;
  PRInt64 mNewVersion;
};

} 

NS_IMPL_THREADSAFE_ISUPPORTS1(OpenDatabaseHelper, nsIRunnable);

nsresult
OpenDatabaseHelper::Init()
{
  nsCString str(mASCIIOrigin);
  str.Append("*");
  str.Append(NS_ConvertUTF16toUTF8(mName));

  nsCOMPtr<nsIAtom> atom = do_GetAtom(str);
  NS_ENSURE_TRUE(atom, NS_ERROR_FAILURE);

  atom.swap(mDatabaseId);
  return NS_OK;
}

nsresult
OpenDatabaseHelper::Dispatch(nsIEventTarget* aTarget)
{
  NS_ASSERTION(mState == eCreated, "We've already been dispatched?");
  mState = eDBWork;

  return aTarget->Dispatch(this, NS_DISPATCH_NORMAL);
}

nsresult
OpenDatabaseHelper::RunImmediately()
{
  NS_ASSERTION(mState == eCreated, "We've already been dispatched?");
  NS_ASSERTION(NS_FAILED(mResultCode),
               "Should only be short-circuiting if we failed!");
  NS_ASSERTION(NS_IsMainThread(), "All hell is about to break lose!");

  mState = eFiringEvents;
  return this->Run();
}

nsresult
OpenDatabaseHelper::DoDatabaseWork()
{
#ifdef DEBUG
  {
    bool correctThread;
    NS_ASSERTION(NS_SUCCEEDED(IndexedDatabaseManager::Get()->IOThread()->
                              IsOnCurrentThread(&correctThread)) &&
                 correctThread,
                 "Running on the wrong thread!");
  }
#endif

  mState = eFiringEvents; 

  if (IndexedDatabaseManager::IsShuttingDown()) {
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  nsCOMPtr<nsIFile> dbFile;
  nsresult rv = GetDatabaseFile(mASCIIOrigin, mName, getter_AddRefs(dbFile));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  rv = dbFile->GetPath(mDatabaseFilePath);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsCOMPtr<nsIFile> dbDirectory;
  rv = dbFile->GetParent(getter_AddRefs(dbDirectory));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  bool exists;
  rv = dbDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  if (exists) {
    bool isDirectory;
    rv = dbDirectory->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
    NS_ENSURE_TRUE(isDirectory, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
  }
  else {
    rv = dbDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
  }

  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "This should never be null!");

  rv = mgr->EnsureQuotaManagementForDirectory(dbDirectory);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsCOMPtr<mozIStorageConnection> connection;
  rv = CreateDatabaseConnection(mName, dbFile, getter_AddRefs(connection));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = connection->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT dataVersion "
    "FROM database"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  if (!hasResult) {
    NS_ERROR("Database has no dataVersion!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  PRInt64 dataVersion;
  rv = stmt->GetInt64(0, &dataVersion);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  if (dataVersion > JS_STRUCTURED_CLONE_VERSION) {
    NS_ERROR("Bad data version!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  if (dataVersion < JS_STRUCTURED_CLONE_VERSION) {
    
    NS_NOTYETIMPLEMENTED("Implement me!");
  }

  rv = IDBFactory::LoadDatabaseInformation(connection, mDatabaseId, &mCurrentVersion,
                                           mObjectStores);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  for (PRUint32 i = 0; i < mObjectStores.Length(); i++) {
    nsAutoPtr<ObjectStoreInfo>& objectStoreInfo = mObjectStores[i];
    for (PRUint32 j = 0; j < objectStoreInfo->indexes.Length(); j++) {
      IndexInfo& indexInfo = objectStoreInfo->indexes[j];
      mLastIndexId = NS_MAX(indexInfo.id, mLastIndexId);
    }
    mLastObjectStoreId = NS_MAX(objectStoreInfo->id, mLastObjectStoreId);
  }

  

  
  if (!mRequestedVersion) {
    
    
    if (mCurrentVersion == 0) {
      mRequestedVersion = 1;
    }
    else {
      
      mRequestedVersion = mCurrentVersion;
    }
  }

  if (mCurrentVersion > mRequestedVersion) {
    return NS_ERROR_DOM_INDEXEDDB_VERSION_ERR;
  }

  if (mCurrentVersion != mRequestedVersion) {
    mState = eSetVersionPending;
  }

  return NS_OK;
}

nsresult
OpenDatabaseHelper::StartSetVersion()
{
  NS_ASSERTION(mState == eSetVersionPending, "Why are we here?");

  
  mState = eFiringEvents;

  nsresult rv = EnsureSuccessResult();
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<nsString> storesToOpen;
  nsRefPtr<IDBTransaction> transaction =
    IDBTransaction::Create(mDatabase, storesToOpen,
                           IDBTransaction::VERSION_CHANGE,
                           kDefaultDatabaseTimeoutSeconds, true);
  NS_ENSURE_TRUE(transaction, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsRefPtr<SetVersionHelper> helper =
    new SetVersionHelper(transaction, mOpenDBRequest, this, mRequestedVersion,
                         mCurrentVersion);

  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "This should never be null!");

  rv = mgr->AcquireExclusiveAccess(mDatabase, helper,
                                   &SetVersionHelper::QueueVersionChange,
                                   helper);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  
  
  mState = eSetVersionPending;

  return NS_OK;
}

NS_IMETHODIMP
OpenDatabaseHelper::Run()
{
  NS_ASSERTION(mState != eCreated, "Dispatch was not called?!?");

  if (NS_IsMainThread()) {
    
    if (mState == eSetVersionPending) {
      nsresult rv = StartSetVersion();

      if (NS_SUCCEEDED(rv)) {
        return rv;
      }

      SetError(rv);
      
    }

    
    
    NS_ASSERTION(mState == eFiringEvents ||
                 mState == eSetVersionCompleted, "Why are we here?");

    if (mState == eSetVersionCompleted) {
      
      
      
      
      mDatabase->ExitSetVersionTransaction();

      mState = eFiringEvents;
    } else {
      
      
      
      

      nsresult rv = mOpenDBRequest->NotifyHelperCompleted(this);
      if (NS_SUCCEEDED(mResultCode) && NS_FAILED(rv)) {
        mResultCode = rv;
      }
    }

    NS_ASSERTION(mState == eFiringEvents, "Why are we here?");

    if (NS_FAILED(mResultCode)) {
      DispatchErrorEvent();
    } else {
      DispatchSuccessEvent();
    }

    IndexedDatabaseManager* manager = IndexedDatabaseManager::Get();
    NS_ASSERTION(manager, "This should never be null!");

    manager->AllowNextSynchronizedOp(mASCIIOrigin, mDatabaseId);

    ReleaseMainThreadObjects();

    return NS_OK;
  }

  
  NS_ASSERTION(mState == eDBWork, "Why are we here?");
  mResultCode = DoDatabaseWork();
  NS_ASSERTION(mState != eDBWork, "We should be doing something else now.");

  return NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
}

nsresult
OpenDatabaseHelper::EnsureSuccessResult()
{
  DatabaseInfo* dbInfo;
  if (DatabaseInfo::Get(mDatabaseId, &dbInfo)) {
    NS_ADDREF(dbInfo);

#ifdef DEBUG
    {
      NS_ASSERTION(dbInfo->name == mName &&
                   dbInfo->version == mCurrentVersion &&
                   dbInfo->id == mDatabaseId &&
                   dbInfo->filePath == mDatabaseFilePath,
                   "Metadata mismatch!");

      PRUint32 objectStoreCount = mObjectStores.Length();
      for (PRUint32 index = 0; index < objectStoreCount; index++) {
        nsAutoPtr<ObjectStoreInfo>& info = mObjectStores[index];
        NS_ASSERTION(info->databaseId == mDatabaseId, "Huh?!");

        ObjectStoreInfo* otherInfo;
        NS_ASSERTION(dbInfo->GetObjectStore(info->name, &otherInfo),
                     "ObjectStore not known!");

        NS_ASSERTION(info->name == otherInfo->name &&
                     info->id == otherInfo->id &&
                     info->keyPath == otherInfo->keyPath &&
                     info->autoIncrement == otherInfo->autoIncrement &&
                     info->databaseId == otherInfo->databaseId,
                     "Metadata mismatch!");
        NS_ASSERTION(dbInfo->ContainsStoreName(info->name),
                     "Object store names out of date!");
        NS_ASSERTION(info->indexes.Length() == otherInfo->indexes.Length(),
                     "Bad index length!");

        PRUint32 indexCount = info->indexes.Length();
        for (PRUint32 indexIndex = 0; indexIndex < indexCount; indexIndex++) {
          const IndexInfo& indexInfo = info->indexes[indexIndex];
          const IndexInfo& otherIndexInfo = otherInfo->indexes[indexIndex];
          NS_ASSERTION(indexInfo.id == otherIndexInfo.id,
                       "Bad index id!");
          NS_ASSERTION(indexInfo.name == otherIndexInfo.name,
                       "Bad index name!");
          NS_ASSERTION(indexInfo.keyPath == otherIndexInfo.keyPath,
                       "Bad index keyPath!");
          NS_ASSERTION(indexInfo.unique == otherIndexInfo.unique,
                       "Bad index unique value!");
          NS_ASSERTION(indexInfo.autoIncrement == otherIndexInfo.autoIncrement,
                       "Bad index autoIncrement value!");
        }
      }
    }
#endif

  }
  else {
    nsRefPtr<DatabaseInfo> newInfo(new DatabaseInfo());

    newInfo->name = mName;
    newInfo->id = mDatabaseId;
    newInfo->filePath = mDatabaseFilePath;

    if (!DatabaseInfo::Put(newInfo)) {
      NS_ERROR("Failed to add to hash!");
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    newInfo.forget(&dbInfo);

    nsresult rv = IDBFactory::UpdateDatabaseMetadata(dbInfo, mCurrentVersion,
                                                     mObjectStores);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

    NS_ASSERTION(mObjectStores.IsEmpty(), "Should have swapped!");
  }

  dbInfo->nextObjectStoreId = mLastObjectStoreId + 1;
  dbInfo->nextIndexId = mLastIndexId + 1;

  nsRefPtr<IDBDatabase> database =
    IDBDatabase::Create(mOpenDBRequest->ScriptContext(),
                        mOpenDBRequest->Owner(), dbInfo, mASCIIOrigin);
  if (!database) {
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  NS_ASSERTION(!mDatabase, "Shouldn't have a database yet!");
  mDatabase.swap(database);

  return NS_OK;
}

nsresult
OpenDatabaseHelper::GetSuccessResult(JSContext* aCx,
                                     jsval* aVal)
{
  
  if (!mDatabase) {
    nsresult rv = EnsureSuccessResult();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return WrapNative(aCx, NS_ISUPPORTS_CAST(nsIDOMEventTarget*, mDatabase),
                    aVal);
}

nsresult
OpenDatabaseHelper::NotifySetVersionFinished()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread");
  NS_ASSERTION(mState = eSetVersionPending, "How did we get here?");

  mState = eSetVersionCompleted;
  
  
  return NS_DispatchToCurrentThread(this);
}

void
OpenDatabaseHelper::BlockDatabase()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mDatabase, "This is going bad fast.");

  mDatabase->EnterSetVersionTransaction();
}

void
OpenDatabaseHelper::DispatchSuccessEvent()
{
  NS_ASSERTION(mDatabase, "Doesn't seem very successful to me.");

  nsRefPtr<nsDOMEvent> event =
    CreateGenericEvent(NS_LITERAL_STRING(SUCCESS_EVT_STR));
  if (!event) {
    NS_ERROR("Failed to create event!");
    return;
  }

  bool dummy;
  mOpenDBRequest->DispatchEvent(event, &dummy);
}

void
OpenDatabaseHelper::DispatchErrorEvent()
{
  nsRefPtr<nsDOMEvent> event =
    CreateGenericEvent(NS_LITERAL_STRING(ERROR_EVT_STR));
  if (!event) {
    NS_ERROR("Failed to create event!");
    return;
  }

  PRUint16 errorCode = 0;
  DebugOnly<nsresult> rv =
    mOpenDBRequest->GetErrorCode(&errorCode);
  NS_ASSERTION(NS_SUCCEEDED(rv), "This shouldn't be failing at this point!");
  if (!errorCode) {
    mOpenDBRequest->SetError(mResultCode);
  }

  bool dummy;
  mOpenDBRequest->DispatchEvent(event, &dummy);
}

void
OpenDatabaseHelper::ReleaseMainThreadObjects()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  mOpenDBRequest = nsnull;
  mDatabase = nsnull;

  HelperBase::ReleaseMainThreadObjects();
}

NS_IMPL_ISUPPORTS_INHERITED0(SetVersionHelper, AsyncConnectionHelper);

nsresult
SetVersionHelper::Init()
{
  
  mOpenHelper->BlockDatabase();

  return NS_OK;
}

nsresult
SetVersionHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  NS_ASSERTION(aConnection, "Passing a null connection!");

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE database "
    "SET version = :version"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("version"),
                             mRequestedVersion);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  if (NS_FAILED(stmt->Execute())) {
    return NS_ERROR_DOM_INDEXEDDB_CONSTRAINT_ERR;
  }

  return NS_OK;
}

nsresult
SetVersionHelper::GetSuccessResult(JSContext* aCx,
                                   jsval* aVal)
{
  DatabaseInfo* info = mDatabase->Info();
  info->version = mRequestedVersion;

  NS_ASSERTION(mTransaction, "Better have a transaction!");

  mOpenRequest->SetTransaction(mTransaction);

  return WrapNative(aCx, NS_ISUPPORTS_CAST(nsIDOMEventTarget*, mDatabase),
                    aVal);
}


void
SetVersionHelper::QueueVersionChange(nsTArray<nsRefPtr<IDBDatabase> >& aDatabases,
                                     void* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aDatabases.IsEmpty(), "Why are we here?");

  SetVersionHelper* helper = static_cast<SetVersionHelper*>(aClosure);
  NS_ASSERTION(helper, "Why don't we have a helper?");

  nsRefPtr<VersionChangeEventsRunnable> eventsRunnable =
    new VersionChangeEventsRunnable(helper->mOpenHelper->Database(),
                                    helper->mOpenRequest,
                                    aDatabases,
                                    helper->mCurrentVersion,
                                    helper->mRequestedVersion);

  NS_DispatchToCurrentThread(eventsRunnable);
}

already_AddRefed<nsDOMEvent>
SetVersionHelper::CreateSuccessEvent()
{
  NS_ASSERTION(mCurrentVersion < mRequestedVersion, "Huh?");

  return IDBVersionChangeEvent::CreateUpgradeNeeded(mCurrentVersion,
                                                    mRequestedVersion);
}

nsresult
SetVersionHelper::NotifyTransactionComplete(IDBTransaction* aTransaction)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTransaction, "This is unexpected.");
  NS_ASSERTION(mOpenRequest, "Why don't we have a request?");

  
  nsresult rv = GetResultCode();
  if (NS_FAILED(rv)) {
    mOpenHelper->SetError(rv);
  }

  
  if (aTransaction->IsAborted()) {
    mOpenHelper->SetError(NS_ERROR_DOM_INDEXEDDB_ABORT_ERR);
  }

  mOpenRequest->SetTransaction(nsnull);

  rv = mOpenHelper->NotifySetVersionFinished();
  mOpenHelper = nsnull;

  return rv;
}
