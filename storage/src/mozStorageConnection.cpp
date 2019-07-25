










































#include <stdio.h>

#include "nsError.h"
#include "nsIMutableArray.h"
#include "nsHashSets.h"
#include "nsAutoPtr.h"
#include "nsIMemoryReporter.h"
#include "nsThreadUtils.h"
#include "nsILocalFile.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"

#include "mozIStorageAggregateFunction.h"
#include "mozIStorageCompletionCallback.h"
#include "mozIStorageFunction.h"

#include "mozStorageAsyncStatementExecution.h"
#include "mozStorageSQLFunctions.h"
#include "mozStorageConnection.h"
#include "mozStorageService.h"
#include "mozStorageStatement.h"
#include "mozStorageAsyncStatement.h"
#include "mozStorageArgValueArray.h"
#include "mozStoragePrivateHelpers.h"
#include "mozStorageStatementData.h"
#include "StorageBaseStatementInternal.h"
#include "SQLCollations.h"

#include "prlog.h"
#include "prprf.h"

#define MIN_AVAILABLE_BYTES_PER_CHUNKED_GROWTH 524288000 // 500 MiB



#define MAX_CACHE_SIZE_BYTES 4194304 // 4 MiB


#define DEFAULT_CACHE_SIZE_PAGES 2000

#ifdef PR_LOGGING
PRLogModuleInfo* gStorageLog = nsnull;
#endif

namespace mozilla {
namespace storage {

namespace {




int
sqlite3_T_int(sqlite3_context *aCtx,
              int aValue)
{
  ::sqlite3_result_int(aCtx, aValue);
  return SQLITE_OK;
}

int
sqlite3_T_int64(sqlite3_context *aCtx,
                sqlite3_int64 aValue)
{
  ::sqlite3_result_int64(aCtx, aValue);
  return SQLITE_OK;
}

int
sqlite3_T_double(sqlite3_context *aCtx,
                 double aValue)
{
  ::sqlite3_result_double(aCtx, aValue);
  return SQLITE_OK;
}

int
sqlite3_T_text(sqlite3_context *aCtx,
               const nsCString &aValue)
{
  ::sqlite3_result_text(aCtx,
                        aValue.get(),
                        aValue.Length(),
                        SQLITE_TRANSIENT);
  return SQLITE_OK;
}

int
sqlite3_T_text16(sqlite3_context *aCtx,
                 const nsString &aValue)
{
  ::sqlite3_result_text16(aCtx,
                          aValue.get(),
                          aValue.Length() * 2, 
                          SQLITE_TRANSIENT);
  return SQLITE_OK;
}

int
sqlite3_T_null(sqlite3_context *aCtx)
{
  ::sqlite3_result_null(aCtx);
  return SQLITE_OK;
}

int
sqlite3_T_blob(sqlite3_context *aCtx,
               const void *aData,
               int aSize)
{
  ::sqlite3_result_blob(aCtx, aData, aSize, NS_Free);
  return SQLITE_OK;
}

#include "variantToSQLiteT_impl.h"




#ifdef PR_LOGGING
void tracefunc (void *aClosure, const char *aStmt)
{
  PR_LOG(gStorageLog, PR_LOG_DEBUG, ("sqlite3_trace on %p for '%s'", aClosure,
                                     aStmt));
}
#endif

struct FFEArguments
{
    nsISupports *target;
    bool found;
};
PLDHashOperator
findFunctionEnumerator(const nsACString &aKey,
                       Connection::FunctionInfo aData,
                       void *aUserArg)
{
  FFEArguments *args = static_cast<FFEArguments *>(aUserArg);
  if (aData.function == args->target) {
    args->found = true;
    return PL_DHASH_STOP;
  }
  return PL_DHASH_NEXT;
}

PLDHashOperator
copyFunctionEnumerator(const nsACString &aKey,
                       Connection::FunctionInfo aData,
                       void *aUserArg)
{
  NS_PRECONDITION(aData.type == Connection::FunctionInfo::SIMPLE ||
                  aData.type == Connection::FunctionInfo::AGGREGATE,
                  "Invalid function type!");

  Connection *connection = static_cast<Connection *>(aUserArg);
  if (aData.type == Connection::FunctionInfo::SIMPLE) {
    mozIStorageFunction *function =
      static_cast<mozIStorageFunction *>(aData.function.get());
    (void)connection->CreateFunction(aKey, aData.numArgs, function);
  }
  else {
    mozIStorageAggregateFunction *function =
      static_cast<mozIStorageAggregateFunction *>(aData.function.get());
    (void)connection->CreateAggregateFunction(aKey, aData.numArgs, function);
  }

  return PL_DHASH_NEXT;
}

void
basicFunctionHelper(sqlite3_context *aCtx,
                    int aArgc,
                    sqlite3_value **aArgv)
{
  void *userData = ::sqlite3_user_data(aCtx);

  mozIStorageFunction *func = static_cast<mozIStorageFunction *>(userData);

  nsRefPtr<ArgValueArray> arguments(new ArgValueArray(aArgc, aArgv));
  if (!arguments)
      return;

  nsCOMPtr<nsIVariant> result;
  if (NS_FAILED(func->OnFunctionCall(arguments, getter_AddRefs(result)))) {
    NS_WARNING("User function returned error code!");
    ::sqlite3_result_error(aCtx,
                           "User function returned error code",
                           -1);
    return;
  }
  if (variantToSQLiteT(aCtx, result) != SQLITE_OK) {
    NS_WARNING("User function returned invalid data type!");
    ::sqlite3_result_error(aCtx,
                           "User function returned invalid data type",
                           -1);
  }
}

void
aggregateFunctionStepHelper(sqlite3_context *aCtx,
                            int aArgc,
                            sqlite3_value **aArgv)
{
  void *userData = ::sqlite3_user_data(aCtx);
  mozIStorageAggregateFunction *func =
    static_cast<mozIStorageAggregateFunction *>(userData);

  nsRefPtr<ArgValueArray> arguments(new ArgValueArray(aArgc, aArgv));
  if (!arguments)
    return;

  if (NS_FAILED(func->OnStep(arguments)))
    NS_WARNING("User aggregate step function returned error code!");
}

void
aggregateFunctionFinalHelper(sqlite3_context *aCtx)
{
  void *userData = ::sqlite3_user_data(aCtx);
  mozIStorageAggregateFunction *func =
    static_cast<mozIStorageAggregateFunction *>(userData);

  nsRefPtr<nsIVariant> result;
  if (NS_FAILED(func->OnFinal(getter_AddRefs(result)))) {
    NS_WARNING("User aggregate final function returned error code!");
    ::sqlite3_result_error(aCtx,
                           "User aggregate final function returned error code",
                           -1);
    return;
  }

  if (variantToSQLiteT(aCtx, result) != SQLITE_OK) {
    NS_WARNING("User aggregate final function returned invalid data type!");
    ::sqlite3_result_error(aCtx,
                           "User aggregate final function returned invalid data type",
                           -1);
  }
}





class UnlockNotification
{
public:
  UnlockNotification()
  : mMutex("UnlockNotification mMutex")
  , mCondVar(mMutex, "UnlockNotification condVar")
  , mSignaled(false)
  {
  }

  void Wait()
  {
    MutexAutoLock lock(mMutex);
    while (!mSignaled) {
      (void)mCondVar.Wait();
    }
  }

  void Signal()
  {
    MutexAutoLock lock(mMutex);
    mSignaled = true;
    (void)mCondVar.Notify();
  }

private:
  Mutex mMutex;
  CondVar mCondVar;
  bool mSignaled;
};

void
UnlockNotifyCallback(void **aArgs,
                     int aArgsSize)
{
  for (int i = 0; i < aArgsSize; i++) {
    UnlockNotification *notification =
      static_cast<UnlockNotification *>(aArgs[i]);
    notification->Signal();
  }
}

int
WaitForUnlockNotify(sqlite3* aDatabase)
{
  UnlockNotification notification;
  int srv = ::sqlite3_unlock_notify(aDatabase, UnlockNotifyCallback,
                                    &notification);
  MOZ_ASSERT(srv == SQLITE_LOCKED || srv == SQLITE_OK);
  if (srv == SQLITE_OK) {
    notification.Wait();
  }

  return srv;
}

} 




namespace {

class AsyncCloseConnection : public nsRunnable
{
public:
  AsyncCloseConnection(Connection *aConnection,
                       nsIEventTarget *aCallingThread,
                       nsIRunnable *aCallbackEvent)
  : mConnection(aConnection)
  , mCallingThread(aCallingThread)
  , mCallbackEvent(aCallbackEvent)
  {
  }

  NS_METHOD Run()
  {
    
    
    
    bool onCallingThread = false;
    (void)mCallingThread->IsOnCurrentThread(&onCallingThread);
    if (!onCallingThread) {
      (void)mCallingThread->Dispatch(this, NS_DISPATCH_NORMAL);
      return NS_OK;
    }

    (void)mConnection->internalClose();
    if (mCallbackEvent)
      (void)mCallingThread->Dispatch(mCallbackEvent, NS_DISPATCH_NORMAL);

    
    
    
    
    
    
    
    
    
    mConnection = nsnull;
    mCallbackEvent = nsnull;

    return NS_OK;
  }
private:
  nsRefPtr<Connection> mConnection;
  nsCOMPtr<nsIEventTarget> mCallingThread;
  nsCOMPtr<nsIRunnable> mCallbackEvent;
};

} 




class StorageMemoryReporter : public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS

  enum ReporterType {
    Cache_Used,
    Schema_Used,
    Stmt_Used
  };

  StorageMemoryReporter(sqlite3 *aDBConn,
                        const nsCString &aFileName,
                        ReporterType aType)
  : mDBConn(aDBConn)
  , mFileName(aFileName)
  , mType(aType)
  , mHasLeaked(false)
  {
  }

  NS_IMETHOD GetProcess(nsACString &process)
  {
    process.Truncate();
    return NS_OK;
  }

  NS_IMETHOD GetPath(nsACString &path)
  {
    path.AssignLiteral("explicit/storage/sqlite/");
    path.Append(mFileName);
    if (mHasLeaked) {
      path.AppendLiteral("-LEAKED");
    }

    if (mType == Cache_Used) {
      path.AppendLiteral("/cache-used");
    }
    else if (mType == Schema_Used) {
      path.AppendLiteral("/schema-used");
    }
    else if (mType == Stmt_Used) {
      path.AppendLiteral("/stmt-used");
    }
    return NS_OK;
  }

  NS_IMETHOD GetKind(PRInt32 *kind)
  {
    *kind = KIND_HEAP;
    return NS_OK;
  }

  NS_IMETHOD GetUnits(PRInt32 *units)
  {
    *units = UNITS_BYTES;
    return NS_OK;
  }

  NS_IMETHOD GetAmount(PRInt64 *amount)
  {
    int type = 0;
    if (mType == Cache_Used) {
      type = SQLITE_DBSTATUS_CACHE_USED;
    }
    else if (mType == Schema_Used) {
      type = SQLITE_DBSTATUS_SCHEMA_USED;
    }
    else if (mType == Stmt_Used) {
      type = SQLITE_DBSTATUS_STMT_USED;
    }

    int cur=0, max=0;
    int rc = ::sqlite3_db_status(mDBConn, type, &cur, &max, 0);
    *amount = cur;
    return convertResultCode(rc);
  }

  NS_IMETHOD GetDescription(nsACString &desc)
  {
    if (mType == Cache_Used) {
      desc.AssignLiteral("Memory (approximate) used by all pager caches used "
                         "by connections to this database.");
    }
    else if (mType == Schema_Used) {
      desc.AssignLiteral("Memory (approximate) used to store the schema "
                         "for all databases associated with connections to "
                         "this database.");
    }
    else if (mType == Stmt_Used) {
      desc.AssignLiteral("Memory (approximate) used by all prepared statements "
                         "used by connections to this database.");
    }
    return NS_OK;
  }

  
  void markAsLeaked()
  {
    mHasLeaked = true;
  }

private:
  sqlite3 *mDBConn;
  nsCString mFileName;
  ReporterType mType;
  bool mHasLeaked;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(
  StorageMemoryReporter
, nsIMemoryReporter
)




Connection::Connection(Service *aService,
                       int aFlags)
: sharedAsyncExecutionMutex("Connection::sharedAsyncExecutionMutex")
, sharedDBMutex("Connection::sharedDBMutex")
, threadOpenedOn(do_GetCurrentThread())
, mDBConn(nsnull)
, mAsyncExecutionThreadShuttingDown(false)
, mTransactionInProgress(false)
, mProgressHandler(nsnull)
, mFlags(aFlags)
, mStorageService(aService)
{
  mFunctions.Init();
  mStorageService->registerConnection(this);
}

Connection::~Connection()
{
  (void)Close();
}

NS_IMPL_THREADSAFE_ADDREF(Connection)
NS_IMPL_THREADSAFE_QUERY_INTERFACE2(
  Connection,
  mozIStorageConnection,
  nsIInterfaceRequestor
)



NS_IMETHODIMP_(nsrefcnt) Connection::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  nsrefcnt count = NS_AtomicDecrementRefcnt(mRefCnt);
  NS_LOG_RELEASE(this, count, "Connection");
  if (1 == count) {
    
    
    
    mStorageService->unregisterConnection(this);
  } else if (0 == count) {
    mRefCnt = 1; 
    
    
    delete (this);
    return 0;
  }
  return count;
}

nsIEventTarget *
Connection::getAsyncExecutionTarget()
{
  MutexAutoLock lockedScope(sharedAsyncExecutionMutex);

  
  
  if (mAsyncExecutionThreadShuttingDown)
    return nsnull;

  if (!mAsyncExecutionThread) {
    nsresult rv = ::NS_NewThread(getter_AddRefs(mAsyncExecutionThread));
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create async thread.");
      return nsnull;
    }
  }

  return mAsyncExecutionThread;
}

nsresult
Connection::initialize(nsIFile *aDatabaseFile,
                       const char* aVFSName)
{
  NS_ASSERTION (!mDBConn, "Initialize called on already opened database!");

  int srv;
  nsresult rv;

  mDatabaseFile = aDatabaseFile;

  if (aDatabaseFile) {
    nsAutoString path;
    rv = aDatabaseFile->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    srv = ::sqlite3_open_v2(NS_ConvertUTF16toUTF8(path).get(), &mDBConn, mFlags,
                            aVFSName);
  }
  else {
    
    srv = ::sqlite3_open_v2(":memory:", &mDBConn, mFlags, aVFSName);
  }
  if (srv != SQLITE_OK) {
    mDBConn = nsnull;
    return convertResultCode(srv);
  }

  
  sharedDBMutex.initWithMutex(sqlite3_db_mutex(mDBConn));

#ifdef PR_LOGGING
  if (!gStorageLog)
    gStorageLog = ::PR_NewLogModule("mozStorage");

  ::sqlite3_trace(mDBConn, tracefunc, this);

  nsCAutoString leafName(":memory");
  if (aDatabaseFile)
    (void)aDatabaseFile->GetNativeLeafName(leafName);
  PR_LOG(gStorageLog, PR_LOG_NOTICE, ("Opening connection to '%s' (%p)",
                                      leafName.get(), this));
#endif

  
  
  
  PRInt64 pageSize = DEFAULT_PAGE_SIZE;
  nsCAutoString pageSizeQuery("PRAGMA page_size = ");
  pageSizeQuery.AppendInt(pageSize);
  rv = ExecuteSimpleSQL(pageSizeQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  
  sqlite3_stmt *stmt;
  srv = prepareStatement(NS_LITERAL_CSTRING("PRAGMA page_size"), &stmt);
  if (srv == SQLITE_OK) {
    if (SQLITE_ROW == stepStatement(stmt)) {
      pageSize = ::sqlite3_column_int64(stmt, 0);
    }
    (void)::sqlite3_finalize(stmt);
  }

  
  
  
  nsCAutoString cacheSizeQuery("PRAGMA cache_size = ");
  cacheSizeQuery.AppendInt(NS_MIN(DEFAULT_CACHE_SIZE_PAGES,
                                  PRInt32(MAX_CACHE_SIZE_BYTES / pageSize)));
  srv = ::sqlite3_exec(mDBConn, cacheSizeQuery.get(), NULL, NULL, NULL);
  if (srv != SQLITE_OK) {
    ::sqlite3_close(mDBConn);
    mDBConn = nsnull;
    return convertResultCode(srv);
  }

  
  srv = registerFunctions(mDBConn);
  if (srv != SQLITE_OK) {
    ::sqlite3_close(mDBConn);
    mDBConn = nsnull;
    return convertResultCode(srv);
  }

  
  srv = registerCollations(mDBConn, mStorageService);
  if (srv != SQLITE_OK) {
    ::sqlite3_close(mDBConn);
    mDBConn = nsnull;
    return convertResultCode(srv);
  }

  
  switch (Service::getSynchronousPref()) {
    case 2:
      (void)ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "PRAGMA synchronous = FULL;"));
      break;
    case 0:
      (void)ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "PRAGMA synchronous = OFF;"));
      break;
    case 1:
    default:
      (void)ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "PRAGMA synchronous = NORMAL;"));
      break;
  }

  return NS_OK;
}

nsresult
Connection::databaseElementExists(enum DatabaseElementType aElementType,
                                  const nsACString &aElementName,
                                  bool *_exists)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  nsCAutoString query("SELECT name FROM sqlite_master WHERE type = '");
  switch (aElementType) {
    case INDEX:
      query.Append("index");
      break;
    case TABLE:
      query.Append("table");
      break;
  }
  query.Append("' AND name ='");
  query.Append(aElementName);
  query.Append("'");

  sqlite3_stmt *stmt;
  int srv = prepareStatement(query, &stmt);
  if (srv != SQLITE_OK)
    return convertResultCode(srv);

  srv = stepStatement(stmt);
  
  (void)::sqlite3_finalize(stmt);

  if (srv == SQLITE_ROW) {
    *_exists = true;
    return NS_OK;
  }
  if (srv == SQLITE_DONE) {
    *_exists = false;
    return NS_OK;
  }

  return convertResultCode(srv);
}

bool
Connection::findFunctionByInstance(nsISupports *aInstance)
{
  sharedDBMutex.assertCurrentThreadOwns();
  FFEArguments args = { aInstance, false };
  (void)mFunctions.EnumerateRead(findFunctionEnumerator, &args);
  return args.found;
}

 int
Connection::sProgressHelper(void *aArg)
{
  Connection *_this = static_cast<Connection *>(aArg);
  return _this->progressHandler();
}

int
Connection::progressHandler()
{
  sharedDBMutex.assertCurrentThreadOwns();
  if (mProgressHandler) {
    bool result;
    nsresult rv = mProgressHandler->OnProgress(this, &result);
    if (NS_FAILED(rv)) return 0; 
    return result ? 1 : 0;
  }
  return 0;
}

nsresult
Connection::setClosedState()
{
  
  bool onOpenedThread;
  nsresult rv = threadOpenedOn->IsOnCurrentThread(&onOpenedThread);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!onOpenedThread) {
    NS_ERROR("Must close the database on the thread that you opened it with!");
    return NS_ERROR_UNEXPECTED;
  }

  
  
  {
    MutexAutoLock lockedScope(sharedAsyncExecutionMutex);
    NS_ENSURE_FALSE(mAsyncExecutionThreadShuttingDown, NS_ERROR_UNEXPECTED);
    mAsyncExecutionThreadShuttingDown = true;
  }

  return NS_OK;
}

nsresult
Connection::internalClose()
{
#ifdef DEBUG
  
  NS_ASSERTION(mDBConn, "Database connection is already null!");

  { 
    MutexAutoLock lockedScope(sharedAsyncExecutionMutex);
    NS_ASSERTION(mAsyncExecutionThreadShuttingDown,
                 "Did not call setClosedState!");
  }

  { 
    bool onOpenedThread = false;
    (void)threadOpenedOn->IsOnCurrentThread(&onOpenedThread);
    NS_ASSERTION(onOpenedThread,
                 "Not called on the thread the database was opened on!");
  }
#endif

#ifdef PR_LOGGING
  nsCAutoString leafName(":memory");
  if (mDatabaseFile)
      (void)mDatabaseFile->GetNativeLeafName(leafName);
  PR_LOG(gStorageLog, PR_LOG_NOTICE, ("Closing connection to '%s'",
                                      leafName.get()));
#endif

#ifdef DEBUG
  
  sqlite3_stmt *stmt = NULL;
  while ((stmt = ::sqlite3_next_stmt(mDBConn, stmt))) {
    char *msg = ::PR_smprintf("SQL statement '%s' was not finalized",
                              ::sqlite3_sql(stmt));
    NS_WARNING(msg);
    ::PR_smprintf_free(msg);
  }
#endif

  int srv = ::sqlite3_close(mDBConn);
  NS_ASSERTION(srv == SQLITE_OK,
               "sqlite3_close failed. There are probably outstanding statements that are listed above!");

  mDBConn = NULL;
  return convertResultCode(srv);
}

nsCString
Connection::getFilename()
{
  nsCString leafname(":memory:");
  if (mDatabaseFile) {
    (void)mDatabaseFile->GetNativeLeafName(leafname);
  }
  return leafname;
}

int
Connection::stepStatement(sqlite3_stmt *aStatement)
{
  bool checkedMainThread = false;
  TimeStamp startTime = TimeStamp::Now();

  (void)::sqlite3_extended_result_codes(mDBConn, 1);

  int srv;
  while ((srv = ::sqlite3_step(aStatement)) == SQLITE_LOCKED_SHAREDCACHE) {
    if (!checkedMainThread) {
      checkedMainThread = true;
      if (::NS_IsMainThread()) {
        NS_WARNING("We won't allow blocking on the main thread!");
        break;
      }
    }

    srv = WaitForUnlockNotify(mDBConn);
    if (srv != SQLITE_OK) {
      break;
    }

    ::sqlite3_reset(aStatement);
  }

  
  TimeDuration duration = TimeStamp::Now() - startTime;
  if (duration.ToMilliseconds() >= Telemetry::kSlowStatementThreshold) {
    nsDependentCString statementString(::sqlite3_sql(aStatement));
    Telemetry::RecordSlowSQLStatement(statementString, getFilename(),
                                      duration.ToMilliseconds());
  }

  (void)::sqlite3_extended_result_codes(mDBConn, 0);
  
  return srv & 0xFF;
}

int
Connection::prepareStatement(const nsCString &aSQL,
                             sqlite3_stmt **_stmt)
{
  bool checkedMainThread = false;

  (void)::sqlite3_extended_result_codes(mDBConn, 1);

  int srv;
  while((srv = ::sqlite3_prepare_v2(mDBConn, aSQL.get(), -1, _stmt, NULL)) ==
        SQLITE_LOCKED_SHAREDCACHE) {
    if (!checkedMainThread) {
      checkedMainThread = true;
      if (::NS_IsMainThread()) {
        NS_WARNING("We won't allow blocking on the main thread!");
        break;
      }
    }

    srv = WaitForUnlockNotify(mDBConn);
    if (srv != SQLITE_OK) {
      break;
    }
  }

  if (srv != SQLITE_OK) {
    nsCString warnMsg;
    warnMsg.AppendLiteral("The SQL statement '");
    warnMsg.Append(aSQL);
    warnMsg.AppendLiteral("' could not be compiled due to an error: ");
    warnMsg.Append(::sqlite3_errmsg(mDBConn));

#ifdef DEBUG
    NS_WARNING(warnMsg.get());
#endif
#ifdef PR_LOGGING
    PR_LOG(gStorageLog, PR_LOG_ERROR, ("%s", warnMsg.get()));
#endif
  }

  (void)::sqlite3_extended_result_codes(mDBConn, 0);
  
  return srv & 0xFF;
}




NS_IMETHODIMP
Connection::GetInterface(const nsIID &aIID,
                         void **_result)
{
  if (aIID.Equals(NS_GET_IID(nsIEventTarget))) {
    nsIEventTarget *background = getAsyncExecutionTarget();
    NS_IF_ADDREF(background);
    *_result = background;
    return NS_OK;
  }
  return NS_ERROR_NO_INTERFACE;
}




NS_IMETHODIMP
Connection::Close()
{
  if (!mDBConn)
    return NS_ERROR_NOT_INITIALIZED;

  { 
    
    
    
    MutexAutoLock lockedScope(sharedAsyncExecutionMutex);
    bool asyncCloseWasCalled = !mAsyncExecutionThread;
    NS_ENSURE_TRUE(asyncCloseWasCalled, NS_ERROR_UNEXPECTED);
  }

  nsresult rv = setClosedState();
  NS_ENSURE_SUCCESS(rv, rv);

  return internalClose();
}

NS_IMETHODIMP
Connection::AsyncClose(mozIStorageCompletionCallback *aCallback)
{
  if (!mDBConn)
    return NS_ERROR_NOT_INITIALIZED;

  nsIEventTarget *asyncThread = getAsyncExecutionTarget();
  NS_ENSURE_TRUE(asyncThread, NS_ERROR_UNEXPECTED);

  nsresult rv = setClosedState();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIRunnable> completeEvent;
  if (aCallback) {
    completeEvent = newCompletionEvent(aCallback);
    NS_ENSURE_TRUE(completeEvent, NS_ERROR_OUT_OF_MEMORY);
  }

  
  nsCOMPtr<nsIRunnable> closeEvent =
    new AsyncCloseConnection(this, NS_GetCurrentThread(), completeEvent);
  NS_ENSURE_TRUE(closeEvent, NS_ERROR_OUT_OF_MEMORY);

  rv = asyncThread->Dispatch(closeEvent, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Connection::Clone(bool aReadOnly,
                  mozIStorageConnection **_connection)
{
  if (!mDBConn)
    return NS_ERROR_NOT_INITIALIZED;
  if (!mDatabaseFile)
    return NS_ERROR_UNEXPECTED;

  int flags = mFlags;
  if (aReadOnly) {
    
    flags = (~SQLITE_OPEN_READWRITE & flags) | SQLITE_OPEN_READONLY;
    
    flags = (~SQLITE_OPEN_CREATE & flags);
  }
  nsRefPtr<Connection> clone = new Connection(mStorageService, flags);
  NS_ENSURE_TRUE(clone, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = clone->initialize(mDatabaseFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  static const char * pragmas[] = {
    "cache_size",
    "temp_store",
    "foreign_keys",
    "journal_size_limit",
    "synchronous",
    "wal_autocheckpoint",
  };
  for (PRUint32 i = 0; i < ArrayLength(pragmas); ++i) {
    
    if (aReadOnly && ::strcmp(pragmas[i], "cache_size") != 0 &&
                     ::strcmp(pragmas[i], "temp_store") != 0) {
      continue;
    }

    nsCAutoString pragmaQuery("PRAGMA ");
    pragmaQuery.Append(pragmas[i]);
    nsCOMPtr<mozIStorageStatement> stmt;
    rv = CreateStatement(pragmaQuery, getter_AddRefs(stmt));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    bool hasResult = false;
    if (stmt && NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
      pragmaQuery.AppendLiteral(" = ");
      pragmaQuery.AppendInt(stmt->AsInt32(0));
      rv = clone->ExecuteSimpleSQL(pragmaQuery);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
    }
  }

  
  (void)mFunctions.EnumerateRead(copyFunctionEnumerator, clone);

  NS_ADDREF(*_connection = clone);
  return NS_OK;
}

NS_IMETHODIMP
Connection::GetConnectionReady(bool *_ready)
{
  *_ready = (mDBConn != nsnull);
  return NS_OK;
}

NS_IMETHODIMP
Connection::GetDatabaseFile(nsIFile **_dbFile)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  NS_IF_ADDREF(*_dbFile = mDatabaseFile);

  return NS_OK;
}

NS_IMETHODIMP
Connection::GetLastInsertRowID(PRInt64 *_id)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  sqlite_int64 id = ::sqlite3_last_insert_rowid(mDBConn);
  *_id = id;

  return NS_OK;
}

NS_IMETHODIMP
Connection::GetLastError(PRInt32 *_error)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  *_error = ::sqlite3_errcode(mDBConn);

  return NS_OK;
}

NS_IMETHODIMP
Connection::GetLastErrorString(nsACString &_errorString)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  const char *serr = ::sqlite3_errmsg(mDBConn);
  _errorString.Assign(serr);

  return NS_OK;
}

NS_IMETHODIMP
Connection::GetSchemaVersion(PRInt32 *_version)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<mozIStorageStatement> stmt;
  (void)CreateStatement(NS_LITERAL_CSTRING("PRAGMA user_version"),
                        getter_AddRefs(stmt));
  NS_ENSURE_TRUE(stmt, NS_ERROR_OUT_OF_MEMORY);

  *_version = 0;
  bool hasResult;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult)
    *_version = stmt->AsInt32(0);

  return NS_OK;
}

NS_IMETHODIMP
Connection::SetSchemaVersion(PRInt32 aVersion)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  nsCAutoString stmt(NS_LITERAL_CSTRING("PRAGMA user_version = "));
  stmt.AppendInt(aVersion);

  return ExecuteSimpleSQL(stmt);
}

NS_IMETHODIMP
Connection::CreateStatement(const nsACString &aSQLStatement,
                            mozIStorageStatement **_stmt)
{
  NS_ENSURE_ARG_POINTER(_stmt);
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  nsRefPtr<Statement> statement(new Statement());
  NS_ENSURE_TRUE(statement, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = statement->initialize(this, aSQLStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  Statement *rawPtr;
  statement.forget(&rawPtr);
  *_stmt = rawPtr;
  return NS_OK;
}

NS_IMETHODIMP
Connection::CreateAsyncStatement(const nsACString &aSQLStatement,
                                 mozIStorageAsyncStatement **_stmt)
{
  NS_ENSURE_ARG_POINTER(_stmt);
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  nsRefPtr<AsyncStatement> statement(new AsyncStatement());
  NS_ENSURE_TRUE(statement, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = statement->initialize(this, aSQLStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  AsyncStatement *rawPtr;
  statement.forget(&rawPtr);
  *_stmt = rawPtr;
  return NS_OK;
}

NS_IMETHODIMP
Connection::ExecuteSimpleSQL(const nsACString &aSQLStatement)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  int srv = ::sqlite3_exec(mDBConn, PromiseFlatCString(aSQLStatement).get(),
                           NULL, NULL, NULL);
  return convertResultCode(srv);
}

NS_IMETHODIMP
Connection::ExecuteAsync(mozIStorageBaseStatement **aStatements,
                         PRUint32 aNumStatements,
                         mozIStorageStatementCallback *aCallback,
                         mozIStoragePendingStatement **_handle)
{
  nsTArray<StatementData> stmts(aNumStatements);
  for (PRUint32 i = 0; i < aNumStatements; i++) {
    nsCOMPtr<StorageBaseStatementInternal> stmt = 
      do_QueryInterface(aStatements[i]);

    
    StatementData data;
    nsresult rv = stmt->getAsynchronousStatementData(data);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(stmt->getOwner() == this,
                 "Statement must be from this database connection!");

    
    NS_ENSURE_TRUE(stmts.AppendElement(data), NS_ERROR_OUT_OF_MEMORY);
  }

  
  return AsyncExecuteStatements::execute(stmts, this, aCallback, _handle);
}

NS_IMETHODIMP
Connection::TableExists(const nsACString &aTableName,
                        bool *_exists)
{
    return databaseElementExists(TABLE, aTableName, _exists);
}

NS_IMETHODIMP
Connection::IndexExists(const nsACString &aIndexName,
                        bool* _exists)
{
    return databaseElementExists(INDEX, aIndexName, _exists);
}

NS_IMETHODIMP
Connection::GetTransactionInProgress(bool *_inProgress)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  *_inProgress = mTransactionInProgress;
  return NS_OK;
}

NS_IMETHODIMP
Connection::BeginTransaction()
{
  return BeginTransactionAs(mozIStorageConnection::TRANSACTION_DEFERRED);
}

NS_IMETHODIMP
Connection::BeginTransactionAs(PRInt32 aTransactionType)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  if (mTransactionInProgress)
    return NS_ERROR_FAILURE;
  nsresult rv;
  switch(aTransactionType) {
    case TRANSACTION_DEFERRED:
      rv = ExecuteSimpleSQL(NS_LITERAL_CSTRING("BEGIN DEFERRED"));
      break;
    case TRANSACTION_IMMEDIATE:
      rv = ExecuteSimpleSQL(NS_LITERAL_CSTRING("BEGIN IMMEDIATE"));
      break;
    case TRANSACTION_EXCLUSIVE:
      rv = ExecuteSimpleSQL(NS_LITERAL_CSTRING("BEGIN EXCLUSIVE"));
      break;
    default:
      return NS_ERROR_ILLEGAL_VALUE;
  }
  if (NS_SUCCEEDED(rv))
    mTransactionInProgress = true;
  return rv;
}

NS_IMETHODIMP
Connection::CommitTransaction()
{
  if (!mDBConn)
    return NS_ERROR_NOT_INITIALIZED;

  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  if (!mTransactionInProgress)
    return NS_ERROR_UNEXPECTED;

  nsresult rv = ExecuteSimpleSQL(NS_LITERAL_CSTRING("COMMIT TRANSACTION"));
  if (NS_SUCCEEDED(rv))
    mTransactionInProgress = false;
  return rv;
}

NS_IMETHODIMP
Connection::RollbackTransaction()
{
  if (!mDBConn)
    return NS_ERROR_NOT_INITIALIZED;

  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  if (!mTransactionInProgress)
    return NS_ERROR_UNEXPECTED;

  nsresult rv = ExecuteSimpleSQL(NS_LITERAL_CSTRING("ROLLBACK TRANSACTION"));
  if (NS_SUCCEEDED(rv))
    mTransactionInProgress = false;
  return rv;
}

NS_IMETHODIMP
Connection::CreateTable(const char *aTableName,
                        const char *aTableSchema)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  char *buf = ::PR_smprintf("CREATE TABLE %s (%s)", aTableName, aTableSchema);
  if (!buf)
    return NS_ERROR_OUT_OF_MEMORY;

  int srv = ::sqlite3_exec(mDBConn, buf, NULL, NULL, NULL);
  ::PR_smprintf_free(buf);

  return convertResultCode(srv);
}

NS_IMETHODIMP
Connection::CreateFunction(const nsACString &aFunctionName,
                           PRInt32 aNumArguments,
                           mozIStorageFunction *aFunction)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  
  
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_ENSURE_FALSE(mFunctions.Get(aFunctionName, NULL), NS_ERROR_FAILURE);

  int srv = ::sqlite3_create_function(mDBConn,
                                      nsPromiseFlatCString(aFunctionName).get(),
                                      aNumArguments,
                                      SQLITE_ANY,
                                      aFunction,
                                      basicFunctionHelper,
                                      NULL,
                                      NULL);
  if (srv != SQLITE_OK)
    return convertResultCode(srv);

  FunctionInfo info = { aFunction,
                        Connection::FunctionInfo::SIMPLE,
                        aNumArguments };
  NS_ENSURE_TRUE(mFunctions.Put(aFunctionName, info),
                 NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
Connection::CreateAggregateFunction(const nsACString &aFunctionName,
                                    PRInt32 aNumArguments,
                                    mozIStorageAggregateFunction *aFunction)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_ENSURE_FALSE(mFunctions.Get(aFunctionName, NULL), NS_ERROR_FAILURE);

  
  
  
  NS_ENSURE_FALSE(findFunctionByInstance(aFunction), NS_ERROR_FAILURE);

  int srv = ::sqlite3_create_function(mDBConn,
                                      nsPromiseFlatCString(aFunctionName).get(),
                                      aNumArguments,
                                      SQLITE_ANY,
                                      aFunction,
                                      NULL,
                                      aggregateFunctionStepHelper,
                                      aggregateFunctionFinalHelper);
  if (srv != SQLITE_OK)
    return convertResultCode(srv);

  FunctionInfo info = { aFunction,
                        Connection::FunctionInfo::AGGREGATE,
                        aNumArguments };
  NS_ENSURE_TRUE(mFunctions.Put(aFunctionName, info),
                 NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
Connection::RemoveFunction(const nsACString &aFunctionName)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_ENSURE_TRUE(mFunctions.Get(aFunctionName, NULL), NS_ERROR_FAILURE);

  int srv = ::sqlite3_create_function(mDBConn,
                                      nsPromiseFlatCString(aFunctionName).get(),
                                      0,
                                      SQLITE_ANY,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);
  if (srv != SQLITE_OK)
    return convertResultCode(srv);

  mFunctions.Remove(aFunctionName);

  return NS_OK;
}

NS_IMETHODIMP
Connection::SetProgressHandler(PRInt32 aGranularity,
                               mozIStorageProgressHandler *aHandler,
                               mozIStorageProgressHandler **_oldHandler)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_IF_ADDREF(*_oldHandler = mProgressHandler);

  if (!aHandler || aGranularity <= 0) {
    aHandler = nsnull;
    aGranularity = 0;
  }
  mProgressHandler = aHandler;
  ::sqlite3_progress_handler(mDBConn, aGranularity, sProgressHelper, this);

  return NS_OK;
}

NS_IMETHODIMP
Connection::RemoveProgressHandler(mozIStorageProgressHandler **_oldHandler)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_IF_ADDREF(*_oldHandler = mProgressHandler);

  mProgressHandler = nsnull;
  ::sqlite3_progress_handler(mDBConn, 0, NULL, NULL);

  return NS_OK;
}

NS_IMETHODIMP
Connection::SetGrowthIncrement(PRInt32 aChunkSize, const nsACString &aDatabaseName)
{
  
  
  
#if !defined(ANDROID) && !defined(MOZ_PLATFORM_MAEMO)
  
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(mDatabaseFile);
  NS_ENSURE_STATE(localFile);
  PRInt64 bytesAvailable;
  nsresult rv = localFile->GetDiskSpaceAvailable(&bytesAvailable);
  NS_ENSURE_SUCCESS(rv, rv);
  if (bytesAvailable < MIN_AVAILABLE_BYTES_PER_CHUNKED_GROWTH) {
    return NS_ERROR_FILE_TOO_BIG;
  }

  (void)::sqlite3_file_control(mDBConn,
                               aDatabaseName.Length() ? nsPromiseFlatCString(aDatabaseName).get() : NULL,
                               SQLITE_FCNTL_CHUNK_SIZE,
                               &aChunkSize);
#endif
  return NS_OK;
}

} 
} 
