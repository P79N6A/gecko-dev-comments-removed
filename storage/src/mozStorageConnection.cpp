





#include <stdio.h>

#include "nsError.h"
#include "nsIMutableArray.h"
#include "nsAutoPtr.h"
#include "nsIMemoryReporter.h"
#include "nsThreadUtils.h"
#include "nsIFile.h"
#include "nsIFileURL.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "mozilla/Attributes.h"

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
#include "FileSystemModule.h"
#include "mozStorageHelper.h"
#include "GeckoProfiler.h"

#include "prlog.h"
#include "prprf.h"
#include "nsProxyRelease.h"
#include <algorithm>

#define MIN_AVAILABLE_BYTES_PER_CHUNKED_GROWTH 524288000 // 500 MiB


#define MAX_CACHE_SIZE_KIBIBYTES 2048 // 2 MiB

#ifdef PR_LOGGING
PRLogModuleInfo* gStorageLog = nullptr;
#endif



#ifdef DEBUG
#define CHECK_MAINTHREAD_ABUSE() \
  do { \
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread(); \
    NS_WARN_IF_FALSE(threadOpenedOn == mainThread || !NS_IsMainThread(), \
               "Using Storage synchronous API on main-thread, but the connection was opened on another thread."); \
  } while(0)
#else
#define CHECK_MAINTHREAD_ABUSE() do { /* Nothing */ } while(0)
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
  ::sqlite3_result_blob(aCtx, aData, aSize, free);
  return SQLITE_OK;
}

#include "variantToSQLiteT_impl.h"




struct Module
{
  const char* name;
  int (*registerFunc)(sqlite3*, const char*);
};

Module gModules[] = {
  { "filesystem", RegisterFileSystemModule }
};




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
  int retcode = variantToSQLiteT(aCtx, result);
  if (retcode == SQLITE_IGNORE) {
    ::sqlite3_result_int(aCtx, SQLITE_IGNORE);
  } else if (retcode != SQLITE_OK) {
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

class AsyncCloseConnection final: public nsRunnable
{
public:
  AsyncCloseConnection(Connection *aConnection,
                       sqlite3 *aNativeConnection,
                       nsIRunnable *aCallbackEvent,
                       already_AddRefed<nsIThread> aAsyncExecutionThread)
  : mConnection(aConnection)
  , mNativeConnection(aNativeConnection)
  , mCallbackEvent(aCallbackEvent)
  , mAsyncExecutionThread(aAsyncExecutionThread)
  {
  }

  NS_METHOD Run()
  {
#ifdef DEBUG
    
    bool onAsyncThread = false;
    (void)mAsyncExecutionThread->IsOnCurrentThread(&onAsyncThread);
    MOZ_ASSERT(onAsyncThread);
#endif 

    nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethodWithArg<nsCOMPtr<nsIThread>>
      (mConnection, &Connection::shutdownAsyncThread, mAsyncExecutionThread);
    (void)NS_DispatchToMainThread(event);

    
    (void)mConnection->internalClose(mNativeConnection);

    
    if (mCallbackEvent) {
      nsCOMPtr<nsIThread> thread;
      (void)NS_GetMainThread(getter_AddRefs(thread));
      (void)thread->Dispatch(mCallbackEvent, NS_DISPATCH_NORMAL);
    }

    return NS_OK;
  }

  ~AsyncCloseConnection() {
    nsCOMPtr<nsIThread> thread;
    (void)NS_GetMainThread(getter_AddRefs(thread));
    
    Connection *rawConnection = nullptr;
    mConnection.swap(rawConnection);
    (void)NS_ProxyRelease(thread,
                          NS_ISUPPORTS_CAST(mozIStorageConnection *,
                                            rawConnection));
    (void)NS_ProxyRelease(thread, mCallbackEvent);
  }
private:
  nsRefPtr<Connection> mConnection;
  sqlite3 *mNativeConnection;
  nsCOMPtr<nsIRunnable> mCallbackEvent;
  nsCOMPtr<nsIThread> mAsyncExecutionThread;
};






class AsyncInitializeClone final: public nsRunnable
{
public:
  







  AsyncInitializeClone(Connection* aConnection,
                       Connection* aClone,
                       const bool aReadOnly,
                       mozIStorageCompletionCallback* aCallback)
    : mConnection(aConnection)
    , mClone(aClone)
    , mReadOnly(aReadOnly)
    , mCallback(aCallback)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run() {
    MOZ_ASSERT (NS_GetCurrentThread() == mClone->getAsyncExecutionTarget());

    nsresult rv = mConnection->initializeClone(mClone, mReadOnly);
    if (NS_FAILED(rv)) {
      return Dispatch(rv, nullptr);
    }
    return Dispatch(NS_OK,
                    NS_ISUPPORTS_CAST(mozIStorageAsyncConnection*, mClone));
  }

private:
  nsresult Dispatch(nsresult aResult, nsISupports* aValue) {
    nsRefPtr<CallbackComplete> event = new CallbackComplete(aResult,
                                                            aValue,
                                                            mCallback.forget());
    return mClone->threadOpenedOn->Dispatch(event, NS_DISPATCH_NORMAL);
  }

  ~AsyncInitializeClone() {
    nsCOMPtr<nsIThread> thread;
    DebugOnly<nsresult> rv = NS_GetMainThread(getter_AddRefs(thread));
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    
    Connection *rawConnection = nullptr;
    mConnection.swap(rawConnection);
    (void)NS_ProxyRelease(thread, NS_ISUPPORTS_CAST(mozIStorageConnection *,
                                                    rawConnection));

    Connection *rawClone = nullptr;
    mClone.swap(rawClone);
    (void)NS_ProxyRelease(thread, NS_ISUPPORTS_CAST(mozIStorageConnection *,
                                                    rawClone));

    
    
    
    mozIStorageCompletionCallback *rawCallback = nullptr;
    mCallback.swap(rawCallback);
    (void)NS_ProxyRelease(thread, rawCallback);
  }

  nsRefPtr<Connection> mConnection;
  nsRefPtr<Connection> mClone;
  const bool mReadOnly;
  nsCOMPtr<mozIStorageCompletionCallback> mCallback;
};

} 




Connection::Connection(Service *aService,
                       int aFlags,
                       bool aAsyncOnly)
: sharedAsyncExecutionMutex("Connection::sharedAsyncExecutionMutex")
, sharedDBMutex("Connection::sharedDBMutex")
, threadOpenedOn(do_GetCurrentThread())
, mDBConn(nullptr)
, mAsyncExecutionThreadShuttingDown(false)
, mAsyncExecutionThreadIsAlive(false)
, mConnectionClosed(false)
, mTransactionInProgress(false)
, mProgressHandler(nullptr)
, mFlags(aFlags)
, mStorageService(aService)
, mAsyncOnly(aAsyncOnly)
{
  mStorageService->registerConnection(this);
}

Connection::~Connection()
{
  (void)Close();

  MOZ_ASSERT(!mAsyncExecutionThread,
             "AsyncClose has not been invoked on this connection!");
  MOZ_ASSERT(!mAsyncExecutionThreadIsAlive,
             "The async execution thread should have been shutdown!");
}

NS_IMPL_ADDREF(Connection)

NS_INTERFACE_MAP_BEGIN(Connection)
  NS_INTERFACE_MAP_ENTRY(mozIStorageAsyncConnection)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(mozIStorageConnection, !mAsyncOnly)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozIStorageConnection)
NS_INTERFACE_MAP_END



NS_IMETHODIMP_(MozExternalRefCountType) Connection::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  nsrefcnt count = --mRefCnt;
  NS_LOG_RELEASE(this, count, "Connection");
  if (1 == count) {
    
    
    
    mStorageService->unregisterConnection(this);
  } else if (0 == count) {
    mRefCnt = 1; 
#if 0 
    NS_ASSERT_OWNINGTHREAD(Connection);
#endif
    delete (this);
    return 0;
  }
  return count;
}

int32_t
Connection::getSqliteRuntimeStatus(int32_t aStatusOption, int32_t* aMaxValue)
{
  MOZ_ASSERT(mDBConn, "A connection must exist at this point");
  int curr = 0, max = 0;
  DebugOnly<int> rc = ::sqlite3_db_status(mDBConn, aStatusOption, &curr, &max, 0);
  MOZ_ASSERT(NS_SUCCEEDED(convertResultCode(rc)));
  if (aMaxValue)
    *aMaxValue = max;
  return curr;
}

nsIEventTarget *
Connection::getAsyncExecutionTarget()
{
  MutexAutoLock lockedScope(sharedAsyncExecutionMutex);

  
  
  if (mAsyncExecutionThreadShuttingDown)
    return nullptr;

  if (!mAsyncExecutionThread) {
    nsresult rv = ::NS_NewThread(getter_AddRefs(mAsyncExecutionThread));
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create async thread.");
      return nullptr;
    }
    static nsThreadPoolNaming naming;
    naming.SetThreadPoolName(NS_LITERAL_CSTRING("mozStorage"),
                             mAsyncExecutionThread);
  }

  mAsyncExecutionThreadIsAlive = true;
  return mAsyncExecutionThread;
}

nsresult
Connection::initialize()
{
  NS_ASSERTION (!mDBConn, "Initialize called on already opened database!");
  PROFILER_LABEL("mozStorageConnection", "initialize",
    js::ProfileEntry::Category::STORAGE);

  
  int srv = ::sqlite3_open_v2(":memory:", &mDBConn, mFlags, nullptr);
  if (srv != SQLITE_OK) {
    mDBConn = nullptr;
    return convertResultCode(srv);
  }

  return initializeInternal(nullptr);
}

nsresult
Connection::initialize(nsIFile *aDatabaseFile)
{
  NS_ASSERTION (aDatabaseFile, "Passed null file!");
  NS_ASSERTION (!mDBConn, "Initialize called on already opened database!");
  PROFILER_LABEL("mozStorageConnection", "initialize",
    js::ProfileEntry::Category::STORAGE);

  mDatabaseFile = aDatabaseFile;

  nsAutoString path;
  nsresult rv = aDatabaseFile->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  int srv = ::sqlite3_open_v2(NS_ConvertUTF16toUTF8(path).get(), &mDBConn,
                              mFlags, nullptr);
  if (srv != SQLITE_OK) {
    mDBConn = nullptr;
    return convertResultCode(srv);
  }

  rv = initializeInternal(aDatabaseFile);
  NS_ENSURE_SUCCESS(rv, rv);

  mDatabaseFile = aDatabaseFile;

  return NS_OK;
}

nsresult
Connection::initialize(nsIFileURL *aFileURL)
{
  NS_ASSERTION (aFileURL, "Passed null file URL!");
  NS_ASSERTION (!mDBConn, "Initialize called on already opened database!");
  PROFILER_LABEL("mozStorageConnection", "initialize",
    js::ProfileEntry::Category::STORAGE);

  nsCOMPtr<nsIFile> databaseFile;
  nsresult rv = aFileURL->GetFile(getter_AddRefs(databaseFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString spec;
  rv = aFileURL->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  int srv = ::sqlite3_open_v2(spec.get(), &mDBConn, mFlags, nullptr);
  if (srv != SQLITE_OK) {
    mDBConn = nullptr;
    return convertResultCode(srv);
  }

  rv = initializeInternal(databaseFile);
  NS_ENSURE_SUCCESS(rv, rv);

  mFileURL = aFileURL;
  mDatabaseFile = databaseFile;

  return NS_OK;
}


nsresult
Connection::initializeInternal(nsIFile* aDatabaseFile)
{
  
  sharedDBMutex.initWithMutex(sqlite3_db_mutex(mDBConn));

#ifdef PR_LOGGING
  if (!gStorageLog)
    gStorageLog = ::PR_NewLogModule("mozStorage");

  
  
  if (PR_LOG_TEST(gStorageLog, PR_LOG_DEBUG)) {
    ::sqlite3_trace(mDBConn, tracefunc, this);

    nsAutoCString leafName(":memory");
    if (aDatabaseFile)
      (void)aDatabaseFile->GetNativeLeafName(leafName);
    PR_LOG(gStorageLog, PR_LOG_NOTICE, ("Opening connection to '%s' (%p)",
                                        leafName.get(), this));
  }
#endif

  int64_t pageSize = Service::getDefaultPageSize();

  
  
  
  nsAutoCString pageSizeQuery(MOZ_STORAGE_UNIQUIFY_QUERY_STR
                              "PRAGMA page_size = ");
  pageSizeQuery.AppendInt(pageSize);
  nsresult rv = ExecuteSimpleSQL(pageSizeQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsAutoCString cacheSizeQuery(MOZ_STORAGE_UNIQUIFY_QUERY_STR
                               "PRAGMA cache_size = ");
  cacheSizeQuery.AppendInt(-MAX_CACHE_SIZE_KIBIBYTES);
  int srv = executeSql(mDBConn, cacheSizeQuery.get());
  if (srv != SQLITE_OK) {
    ::sqlite3_close(mDBConn);
    mDBConn = nullptr;
    return convertResultCode(srv);
  }

  
  srv = registerFunctions(mDBConn);
  if (srv != SQLITE_OK) {
    ::sqlite3_close(mDBConn);
    mDBConn = nullptr;
    return convertResultCode(srv);
  }

  
  srv = registerCollations(mDBConn, mStorageService);
  if (srv != SQLITE_OK) {
    ::sqlite3_close(mDBConn);
    mDBConn = nullptr;
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

  
  
  nsCString query("SELECT name FROM (SELECT * FROM ");
  nsDependentCSubstring element;
  int32_t ind = aElementName.FindChar('.');
  if (ind == kNotFound) {
    element.Assign(aElementName);
  }
  else {
    nsDependentCSubstring db(Substring(aElementName, 0, ind + 1));
    element.Assign(Substring(aElementName, ind + 1, aElementName.Length()));
    query.Append(db);
  }
  query.AppendLiteral("sqlite_master UNION ALL SELECT * FROM sqlite_temp_master) WHERE type = '");

  switch (aElementType) {
    case INDEX:
      query.AppendLiteral("index");
      break;
    case TABLE:
      query.AppendLiteral("table");
      break;
  }
  query.AppendLiteral("' AND name ='");
  query.Append(element);
  query.Append('\'');

  sqlite3_stmt *stmt;
  int srv = prepareStatement(mDBConn, query, &stmt);
  if (srv != SQLITE_OK)
    return convertResultCode(srv);

  srv = stepStatement(mDBConn, stmt);
  
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

  
  
  mDBConn = nullptr;

  return NS_OK;
}

bool
Connection::connectionReady()
{
  return mDBConn != nullptr;
}

bool
Connection::isClosing()
{
  bool shuttingDown = false;
  {
    MutexAutoLock lockedScope(sharedAsyncExecutionMutex);
    shuttingDown = mAsyncExecutionThreadShuttingDown;
  }
  return shuttingDown && !isClosed();
}

bool
Connection::isClosed()
{
  MutexAutoLock lockedScope(sharedAsyncExecutionMutex);
  return mConnectionClosed;
}

void
Connection::shutdownAsyncThread(nsIThread *aThread) {
  MOZ_ASSERT(!mAsyncExecutionThread);
  MOZ_ASSERT(mAsyncExecutionThreadIsAlive);
  MOZ_ASSERT(mAsyncExecutionThreadShuttingDown);

  DebugOnly<nsresult> rv = aThread->Shutdown();
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  mAsyncExecutionThreadIsAlive = false;
}

nsresult
Connection::internalClose(sqlite3 *aNativeConnection)
{
  
  
  
  MOZ_ASSERT(!isClosed());

#ifdef DEBUG
  { 
    MutexAutoLock lockedScope(sharedAsyncExecutionMutex);
    NS_ASSERTION(mAsyncExecutionThreadShuttingDown,
                 "Did not call setClosedState!");
  }
#endif 

#ifdef PR_LOGGING
  nsAutoCString leafName(":memory");
  if (mDatabaseFile)
      (void)mDatabaseFile->GetNativeLeafName(leafName);
  PR_LOG(gStorageLog, PR_LOG_NOTICE, ("Closing connection to '%s'",
                                      leafName.get()));
#endif

  
  
  
  
  
  
  {
    MutexAutoLock lockedScope(sharedAsyncExecutionMutex);
    mConnectionClosed = true;
  }

  
  if (!aNativeConnection)
    return NS_OK;

  int srv = sqlite3_close(aNativeConnection);

  if (srv == SQLITE_BUSY) {
    

    sqlite3_stmt *stmt = nullptr;
    while ((stmt = ::sqlite3_next_stmt(aNativeConnection, stmt))) {
      PR_LOG(gStorageLog, PR_LOG_NOTICE,
             ("Auto-finalizing SQL statement '%s' (%x)",
              ::sqlite3_sql(stmt),
              stmt));

#ifdef DEBUG
      char *msg = ::PR_smprintf("SQL statement '%s' (%x) should have been finalized before closing the connection",
                                ::sqlite3_sql(stmt),
                                stmt);
      NS_WARNING(msg);
      ::PR_smprintf_free(msg);
#endif 

      srv = ::sqlite3_finalize(stmt);

#ifdef DEBUG
      if (srv != SQLITE_OK) {
        char *msg = ::PR_smprintf("Could not finalize SQL statement '%s' (%x)",
                                  ::sqlite3_sql(stmt),
                                  stmt);
        NS_WARNING(msg);
        ::PR_smprintf_free(msg);
      }
#endif 

      
      
      if (srv == SQLITE_OK) {
        stmt = nullptr;
      }
    }

    
    
    srv = ::sqlite3_close(aNativeConnection);

  }

  if (srv != SQLITE_OK) {
    MOZ_ASSERT(srv == SQLITE_OK,
               "sqlite3_close failed. There are probably outstanding statements that are listed above!");
  }

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
Connection::stepStatement(sqlite3 *aNativeConnection, sqlite3_stmt *aStatement)
{
  MOZ_ASSERT(aStatement);
  bool checkedMainThread = false;
  TimeStamp startTime = TimeStamp::Now();

  
  
  
  
  
  if (isClosed())
    return SQLITE_MISUSE;

  (void)::sqlite3_extended_result_codes(aNativeConnection, 1);

  int srv;
  while ((srv = ::sqlite3_step(aStatement)) == SQLITE_LOCKED_SHAREDCACHE) {
    if (!checkedMainThread) {
      checkedMainThread = true;
      if (::NS_IsMainThread()) {
        NS_WARNING("We won't allow blocking on the main thread!");
        break;
      }
    }

    srv = WaitForUnlockNotify(aNativeConnection);
    if (srv != SQLITE_OK) {
      break;
    }

    ::sqlite3_reset(aStatement);
  }

  
  TimeDuration duration = TimeStamp::Now() - startTime;
  const uint32_t threshold =
    NS_IsMainThread() ? Telemetry::kSlowSQLThresholdForMainThread
                      : Telemetry::kSlowSQLThresholdForHelperThreads;
  if (duration.ToMilliseconds() >= threshold) {
    nsDependentCString statementString(::sqlite3_sql(aStatement));
    Telemetry::RecordSlowSQLStatement(statementString, getFilename(),
                                      duration.ToMilliseconds());
  }

  (void)::sqlite3_extended_result_codes(aNativeConnection, 0);
  
  return srv & 0xFF;
}

int
Connection::prepareStatement(sqlite3 *aNativeConnection, const nsCString &aSQL,
                             sqlite3_stmt **_stmt)
{
  
  
  if (isClosed())
    return SQLITE_MISUSE;

  bool checkedMainThread = false;

  (void)::sqlite3_extended_result_codes(aNativeConnection, 1);

  int srv;
  while((srv = ::sqlite3_prepare_v2(aNativeConnection,
                                    aSQL.get(),
                                    -1,
                                    _stmt,
                                    nullptr)) == SQLITE_LOCKED_SHAREDCACHE) {
    if (!checkedMainThread) {
      checkedMainThread = true;
      if (::NS_IsMainThread()) {
        NS_WARNING("We won't allow blocking on the main thread!");
        break;
      }
    }

    srv = WaitForUnlockNotify(aNativeConnection);
    if (srv != SQLITE_OK) {
      break;
    }
  }

  if (srv != SQLITE_OK) {
    nsCString warnMsg;
    warnMsg.AppendLiteral("The SQL statement '");
    warnMsg.Append(aSQL);
    warnMsg.AppendLiteral("' could not be compiled due to an error: ");
    warnMsg.Append(::sqlite3_errmsg(aNativeConnection));

#ifdef DEBUG
    NS_WARNING(warnMsg.get());
#endif
    PR_LOG(gStorageLog, PR_LOG_ERROR, ("%s", warnMsg.get()));
  }

  (void)::sqlite3_extended_result_codes(aNativeConnection, 0);
  
  int rc = srv & 0xFF;
  
  
  
  if (rc == SQLITE_OK && *_stmt == nullptr) {
    return SQLITE_MISUSE;
  }

  return rc;
}


int
Connection::executeSql(sqlite3 *aNativeConnection, const char *aSqlString)
{
  if (isClosed())
    return SQLITE_MISUSE;

  TimeStamp startTime = TimeStamp::Now();
  int srv = ::sqlite3_exec(aNativeConnection, aSqlString, nullptr, nullptr,
                           nullptr);

  
  TimeDuration duration = TimeStamp::Now() - startTime;
  const uint32_t threshold =
    NS_IsMainThread() ? Telemetry::kSlowSQLThresholdForMainThread
                      : Telemetry::kSlowSQLThresholdForHelperThreads;
  if (duration.ToMilliseconds() >= threshold) {
    nsDependentCString statementString(aSqlString);
    Telemetry::RecordSlowSQLStatement(statementString, getFilename(),
                                      duration.ToMilliseconds());
  }

  return srv;
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

  
  
  sqlite3 *nativeConn = mDBConn;
  nsresult rv = setClosedState();
  NS_ENSURE_SUCCESS(rv, rv);

  return internalClose(nativeConn);
}

NS_IMETHODIMP
Connection::AsyncClose(mozIStorageCompletionCallback *aCallback)
{
  if (!NS_IsMainThread()) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  
  
  
  nsIEventTarget *asyncThread = getAsyncExecutionTarget();

  if (!mDBConn && !asyncThread)
    return NS_ERROR_NOT_INITIALIZED;

  
  
  sqlite3 *nativeConn = mDBConn;
  nsresult rv = setClosedState();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIRunnable> completeEvent;
  if (aCallback) {
    completeEvent = newCompletionEvent(aCallback);
  }

  
  nsCOMPtr<nsIRunnable> closeEvent;
  {
    
    MutexAutoLock lockedScope(sharedAsyncExecutionMutex);
    closeEvent = new AsyncCloseConnection(this,
                                          nativeConn,
                                          completeEvent,
                                          mAsyncExecutionThread.forget());
  }

  rv = asyncThread->Dispatch(closeEvent, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
Connection::AsyncClone(bool aReadOnly,
                       mozIStorageCompletionCallback *aCallback)
{
  PROFILER_LABEL("mozStorageConnection", "AsyncClone",
    js::ProfileEntry::Category::STORAGE);

  if (!NS_IsMainThread()) {
    return NS_ERROR_NOT_SAME_THREAD;
  }
  if (!mDBConn)
    return NS_ERROR_NOT_INITIALIZED;
  if (!mDatabaseFile)
    return NS_ERROR_UNEXPECTED;

  int flags = mFlags;
  if (aReadOnly) {
    
    flags = (~SQLITE_OPEN_READWRITE & flags) | SQLITE_OPEN_READONLY;
    
    flags = (~SQLITE_OPEN_CREATE & flags);
  }

  nsRefPtr<Connection> clone = new Connection(mStorageService, flags,
                                              mAsyncOnly);

  nsRefPtr<AsyncInitializeClone> initEvent =
    new AsyncInitializeClone(this, clone, aReadOnly, aCallback);
  nsCOMPtr<nsIEventTarget> target = clone->getAsyncExecutionTarget();
  if (!target) {
    return NS_ERROR_UNEXPECTED;
  }
  return target->Dispatch(initEvent, NS_DISPATCH_NORMAL);
}

nsresult
Connection::initializeClone(Connection* aClone, bool aReadOnly)
{
  nsresult rv = mFileURL ? aClone->initialize(mFileURL)
                         : aClone->initialize(mDatabaseFile);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  static const char * pragmas[] = {
    "cache_size",
    "temp_store",
    "foreign_keys",
    "journal_size_limit",
    "synchronous",
    "wal_autocheckpoint",
    "busy_timeout"
  };
  for (uint32_t i = 0; i < ArrayLength(pragmas); ++i) {
    
    if (aReadOnly && ::strcmp(pragmas[i], "cache_size") != 0 &&
                     ::strcmp(pragmas[i], "temp_store") != 0) {
      continue;
    }

    nsAutoCString pragmaQuery("PRAGMA ");
    pragmaQuery.Append(pragmas[i]);
    nsCOMPtr<mozIStorageStatement> stmt;
    rv = CreateStatement(pragmaQuery, getter_AddRefs(stmt));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    bool hasResult = false;
    if (stmt && NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
      pragmaQuery.AppendLiteral(" = ");
      pragmaQuery.AppendInt(stmt->AsInt32(0));
      rv = aClone->ExecuteSimpleSQL(pragmaQuery);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
    }
  }

  
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  (void)mFunctions.EnumerateRead(copyFunctionEnumerator, aClone);

  return NS_OK;
}

NS_IMETHODIMP
Connection::Clone(bool aReadOnly,
                  mozIStorageConnection **_connection)
{
  MOZ_ASSERT(threadOpenedOn == NS_GetCurrentThread());

  PROFILER_LABEL("mozStorageConnection", "Clone",
    js::ProfileEntry::Category::STORAGE);

  if (!mDBConn)
    return NS_ERROR_NOT_INITIALIZED;
  if (!mDatabaseFile)
    return NS_ERROR_UNEXPECTED;

  int flags = mFlags;
  if (aReadOnly) {
    
    flags = (~SQLITE_OPEN_READWRITE & flags) | SQLITE_OPEN_READONLY;
    
    flags = (~SQLITE_OPEN_CREATE & flags);
  }

  nsRefPtr<Connection> clone = new Connection(mStorageService, flags,
                                              mAsyncOnly);

  nsresult rv = initializeClone(clone, aReadOnly);
  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_IF_ADDREF(*_connection = clone);
  return NS_OK;
}

NS_IMETHODIMP
Connection::GetDefaultPageSize(int32_t *_defaultPageSize)
{
  *_defaultPageSize = Service::getDefaultPageSize();
  return NS_OK;
}

NS_IMETHODIMP
Connection::GetConnectionReady(bool *_ready)
{
  *_ready = connectionReady();
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
Connection::GetLastInsertRowID(int64_t *_id)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  sqlite_int64 id = ::sqlite3_last_insert_rowid(mDBConn);
  *_id = id;

  return NS_OK;
}

NS_IMETHODIMP
Connection::GetAffectedRows(int32_t *_rows)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  *_rows = ::sqlite3_changes(mDBConn);

  return NS_OK;
}

NS_IMETHODIMP
Connection::GetLastError(int32_t *_error)
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
Connection::GetSchemaVersion(int32_t *_version)
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
Connection::SetSchemaVersion(int32_t aVersion)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  nsAutoCString stmt(NS_LITERAL_CSTRING("PRAGMA user_version = "));
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

  nsresult rv = statement->initialize(this, mDBConn, aSQLStatement);
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

  nsresult rv = statement->initialize(this, mDBConn, aSQLStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  AsyncStatement *rawPtr;
  statement.forget(&rawPtr);
  *_stmt = rawPtr;
  return NS_OK;
}

NS_IMETHODIMP
Connection::ExecuteSimpleSQL(const nsACString &aSQLStatement)
{
  CHECK_MAINTHREAD_ABUSE();
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  int srv = executeSql(mDBConn, PromiseFlatCString(aSQLStatement).get());
  return convertResultCode(srv);
}

NS_IMETHODIMP
Connection::ExecuteAsync(mozIStorageBaseStatement **aStatements,
                         uint32_t aNumStatements,
                         mozIStorageStatementCallback *aCallback,
                         mozIStoragePendingStatement **_handle)
{
  nsTArray<StatementData> stmts(aNumStatements);
  for (uint32_t i = 0; i < aNumStatements; i++) {
    nsCOMPtr<StorageBaseStatementInternal> stmt = 
      do_QueryInterface(aStatements[i]);

    
    StatementData data;
    nsresult rv = stmt->getAsynchronousStatementData(data);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(stmt->getOwner() == this,
                 "Statement must be from this database connection!");

    
    NS_ENSURE_TRUE(stmts.AppendElement(data), NS_ERROR_OUT_OF_MEMORY);
  }

  
  return AsyncExecuteStatements::execute(stmts, this, mDBConn, aCallback,
                                         _handle);
}

NS_IMETHODIMP
Connection::ExecuteSimpleSQLAsync(const nsACString &aSQLStatement,
                                  mozIStorageStatementCallback *aCallback,
                                  mozIStoragePendingStatement **_handle)
{
  if (!NS_IsMainThread()) {
    return NS_ERROR_NOT_SAME_THREAD;
  }

  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  nsresult rv = CreateAsyncStatement(aSQLStatement, getter_AddRefs(stmt));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<mozIStoragePendingStatement> pendingStatement;
  rv = stmt->ExecuteAsync(aCallback, getter_AddRefs(pendingStatement));
  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_ADDREF(*_handle = pendingStatement);
  return rv;
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
Connection::BeginTransactionAs(int32_t aTransactionType)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  return beginTransactionInternal(mDBConn, aTransactionType);
}

nsresult
Connection::beginTransactionInternal(sqlite3 *aNativeConnection,
                                     int32_t aTransactionType)
{
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  if (mTransactionInProgress)
    return NS_ERROR_FAILURE;
  nsresult rv;
  switch(aTransactionType) {
    case TRANSACTION_DEFERRED:
      rv = convertResultCode(executeSql(aNativeConnection, "BEGIN DEFERRED"));
      break;
    case TRANSACTION_IMMEDIATE:
      rv = convertResultCode(executeSql(aNativeConnection, "BEGIN IMMEDIATE"));
      break;
    case TRANSACTION_EXCLUSIVE:
      rv = convertResultCode(executeSql(aNativeConnection, "BEGIN EXCLUSIVE"));
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

  return commitTransactionInternal(mDBConn);
}

nsresult
Connection::commitTransactionInternal(sqlite3 *aNativeConnection)
{
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  if (!mTransactionInProgress)
    return NS_ERROR_UNEXPECTED;
  nsresult rv =
    convertResultCode(executeSql(aNativeConnection, "COMMIT TRANSACTION"));
  if (NS_SUCCEEDED(rv))
    mTransactionInProgress = false;
  return rv;
}

NS_IMETHODIMP
Connection::RollbackTransaction()
{
  if (!mDBConn)
    return NS_ERROR_NOT_INITIALIZED;

  return rollbackTransactionInternal(mDBConn);
}

nsresult
Connection::rollbackTransactionInternal(sqlite3 *aNativeConnection)
{
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  if (!mTransactionInProgress)
    return NS_ERROR_UNEXPECTED;

  nsresult rv =
    convertResultCode(executeSql(aNativeConnection, "ROLLBACK TRANSACTION"));
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

  int srv = executeSql(mDBConn, buf);
  ::PR_smprintf_free(buf);

  return convertResultCode(srv);
}

NS_IMETHODIMP
Connection::CreateFunction(const nsACString &aFunctionName,
                           int32_t aNumArguments,
                           mozIStorageFunction *aFunction)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  
  
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_ENSURE_FALSE(mFunctions.Get(aFunctionName, nullptr), NS_ERROR_FAILURE);

  int srv = ::sqlite3_create_function(mDBConn,
                                      nsPromiseFlatCString(aFunctionName).get(),
                                      aNumArguments,
                                      SQLITE_ANY,
                                      aFunction,
                                      basicFunctionHelper,
                                      nullptr,
                                      nullptr);
  if (srv != SQLITE_OK)
    return convertResultCode(srv);

  FunctionInfo info = { aFunction,
                        Connection::FunctionInfo::SIMPLE,
                        aNumArguments };
  mFunctions.Put(aFunctionName, info);

  return NS_OK;
}

NS_IMETHODIMP
Connection::CreateAggregateFunction(const nsACString &aFunctionName,
                                    int32_t aNumArguments,
                                    mozIStorageAggregateFunction *aFunction)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_ENSURE_FALSE(mFunctions.Get(aFunctionName, nullptr), NS_ERROR_FAILURE);

  
  
  
  NS_ENSURE_FALSE(findFunctionByInstance(aFunction), NS_ERROR_FAILURE);

  int srv = ::sqlite3_create_function(mDBConn,
                                      nsPromiseFlatCString(aFunctionName).get(),
                                      aNumArguments,
                                      SQLITE_ANY,
                                      aFunction,
                                      nullptr,
                                      aggregateFunctionStepHelper,
                                      aggregateFunctionFinalHelper);
  if (srv != SQLITE_OK)
    return convertResultCode(srv);

  FunctionInfo info = { aFunction,
                        Connection::FunctionInfo::AGGREGATE,
                        aNumArguments };
  mFunctions.Put(aFunctionName, info);

  return NS_OK;
}

NS_IMETHODIMP
Connection::RemoveFunction(const nsACString &aFunctionName)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_ENSURE_TRUE(mFunctions.Get(aFunctionName, nullptr), NS_ERROR_FAILURE);

  int srv = ::sqlite3_create_function(mDBConn,
                                      nsPromiseFlatCString(aFunctionName).get(),
                                      0,
                                      SQLITE_ANY,
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr);
  if (srv != SQLITE_OK)
    return convertResultCode(srv);

  mFunctions.Remove(aFunctionName);

  return NS_OK;
}

NS_IMETHODIMP
Connection::SetProgressHandler(int32_t aGranularity,
                               mozIStorageProgressHandler *aHandler,
                               mozIStorageProgressHandler **_oldHandler)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  
  SQLiteMutexAutoLock lockedScope(sharedDBMutex);
  NS_IF_ADDREF(*_oldHandler = mProgressHandler);

  if (!aHandler || aGranularity <= 0) {
    aHandler = nullptr;
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

  mProgressHandler = nullptr;
  ::sqlite3_progress_handler(mDBConn, 0, nullptr, nullptr);

  return NS_OK;
}

NS_IMETHODIMP
Connection::SetGrowthIncrement(int32_t aChunkSize, const nsACString &aDatabaseName)
{
  
  
  
#if !defined(ANDROID) && !defined(MOZ_PLATFORM_MAEMO)
  
  int64_t bytesAvailable;
  nsresult rv = mDatabaseFile->GetDiskSpaceAvailable(&bytesAvailable);
  NS_ENSURE_SUCCESS(rv, rv);
  if (bytesAvailable < MIN_AVAILABLE_BYTES_PER_CHUNKED_GROWTH) {
    return NS_ERROR_FILE_TOO_BIG;
  }

  (void)::sqlite3_file_control(mDBConn,
                               aDatabaseName.Length() ? nsPromiseFlatCString(aDatabaseName).get()
                                                      : nullptr,
                               SQLITE_FCNTL_CHUNK_SIZE,
                               &aChunkSize);
#endif
  return NS_OK;
}

NS_IMETHODIMP
Connection::EnableModule(const nsACString& aModuleName)
{
  if (!mDBConn) return NS_ERROR_NOT_INITIALIZED;

  for (size_t i = 0; i < ArrayLength(gModules); i++) {
    struct Module* m = &gModules[i];
    if (aModuleName.Equals(m->name)) {
      int srv = m->registerFunc(mDBConn, m->name);
      if (srv != SQLITE_OK)
        return convertResultCode(srv);

      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

} 
} 
