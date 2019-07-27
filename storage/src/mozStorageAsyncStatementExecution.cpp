





#include "nsAutoPtr.h"

#include "sqlite3.h"

#include "mozIStorageStatementCallback.h"
#include "mozStorageBindingParams.h"
#include "mozStorageHelper.h"
#include "mozStorageResultSet.h"
#include "mozStorageRow.h"
#include "mozStorageConnection.h"
#include "mozStorageError.h"
#include "mozStoragePrivateHelpers.h"
#include "mozStorageStatementData.h"
#include "mozStorageAsyncStatementExecution.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/Telemetry.h"

namespace mozilla {
namespace storage {












#define MAX_MILLISECONDS_BETWEEN_RESULTS 75
#define MAX_ROWS_PER_RESULT 15




namespace {

typedef AsyncExecuteStatements::ExecutionState ExecutionState;
typedef AsyncExecuteStatements::StatementDataArray StatementDataArray;




class CallbackResultNotifier : public nsRunnable
{
public:
  CallbackResultNotifier(mozIStorageStatementCallback *aCallback,
                         mozIStorageResultSet *aResults,
                         AsyncExecuteStatements *aEventStatus) :
      mCallback(aCallback)
    , mResults(aResults)
    , mEventStatus(aEventStatus)
  {
  }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(mCallback, "Trying to notify about results without a callback!");

    if (mEventStatus->shouldNotify()) {
      
      
      
      nsCOMPtr<mozIStorageStatementCallback> callback =
        do_QueryInterface(mCallback);

      (void)mCallback->HandleResult(mResults);
    }

    return NS_OK;
  }

private:
  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<mozIStorageResultSet> mResults;
  nsRefPtr<AsyncExecuteStatements> mEventStatus;
};




class ErrorNotifier : public nsRunnable
{
public:
  ErrorNotifier(mozIStorageStatementCallback *aCallback,
                mozIStorageError *aErrorObj,
                AsyncExecuteStatements *aEventStatus) :
      mCallback(aCallback)
    , mErrorObj(aErrorObj)
    , mEventStatus(aEventStatus)
  {
  }

  NS_IMETHOD Run()
  {
    if (mEventStatus->shouldNotify() && mCallback) {
      
      
      
      nsCOMPtr<mozIStorageStatementCallback> callback =
        do_QueryInterface(mCallback);

      (void)mCallback->HandleError(mErrorObj);
    }

    return NS_OK;
  }

private:
  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<mozIStorageError> mErrorObj;
  nsRefPtr<AsyncExecuteStatements> mEventStatus;
};





class CompletionNotifier : public nsRunnable
{
public:
  




  CompletionNotifier(mozIStorageStatementCallback *aCallback,
                     ExecutionState aReason)
    : mCallback(aCallback)
    , mReason(aReason)
  {
  }

  NS_IMETHOD Run()
  {
    if (mCallback) {
      (void)mCallback->HandleCompletion(mReason);
      NS_RELEASE(mCallback);
    }

    return NS_OK;
  }

private:
  mozIStorageStatementCallback *mCallback;
  ExecutionState mReason;
};

} 





nsresult
AsyncExecuteStatements::execute(StatementDataArray &aStatements,
                                Connection *aConnection,
                                sqlite3 *aNativeConnection,
                                mozIStorageStatementCallback *aCallback,
                                mozIStoragePendingStatement **_stmt)
{
  
  nsRefPtr<AsyncExecuteStatements> event =
    new AsyncExecuteStatements(aStatements, aConnection, aNativeConnection,
                               aCallback);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  
  nsIEventTarget *target = aConnection->getAsyncExecutionTarget();

  
  
  
  
  
  MOZ_ASSERT(target);
  if (!target) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ADDREF(*_stmt = event);
  return NS_OK;
}

AsyncExecuteStatements::AsyncExecuteStatements(StatementDataArray &aStatements,
                                               Connection *aConnection,
                                               sqlite3 *aNativeConnection,
                                               mozIStorageStatementCallback *aCallback)
: mConnection(aConnection)
, mNativeConnection(aNativeConnection)
, mHasTransaction(false)
, mCallback(aCallback)
, mCallingThread(::do_GetCurrentThread())
, mMaxWait(TimeDuration::FromMilliseconds(MAX_MILLISECONDS_BETWEEN_RESULTS))
, mIntervalStart(TimeStamp::Now())
, mState(PENDING)
, mCancelRequested(false)
, mMutex(aConnection->sharedAsyncExecutionMutex)
, mDBMutex(aConnection->sharedDBMutex)
  , mRequestStartDate(TimeStamp::Now())
{
  (void)mStatements.SwapElements(aStatements);
  NS_ASSERTION(mStatements.Length(), "We weren't given any statements!");
  NS_IF_ADDREF(mCallback);
}

AsyncExecuteStatements::~AsyncExecuteStatements()
{
  MOZ_ASSERT(!mHasTransaction, "There should be no transaction at this point");
}

bool
AsyncExecuteStatements::shouldNotify()
{
#ifdef DEBUG
  mMutex.AssertNotCurrentThreadOwns();

  bool onCallingThread = false;
  (void)mCallingThread->IsOnCurrentThread(&onCallingThread);
  NS_ASSERTION(onCallingThread, "runEvent not running on the calling thread!");
#endif

  
  
  
  return !mCancelRequested;
}

bool
AsyncExecuteStatements::bindExecuteAndProcessStatement(StatementData &aData,
                                                       bool aLastStatement)
{
  mMutex.AssertNotCurrentThreadOwns();

  sqlite3_stmt *aStatement = nullptr;
  
  (void)aData.getSqliteStatement(&aStatement);
  NS_ASSERTION(aStatement, "You broke the code; do not call here like that!");
  BindingParamsArray *paramsArray(aData);

  
  bool continueProcessing = true;
  BindingParamsArray::iterator itr = paramsArray->begin();
  BindingParamsArray::iterator end = paramsArray->end();
  while (itr != end && continueProcessing) {
    
    nsCOMPtr<IStorageBindingParamsInternal> bindingInternal = 
      do_QueryInterface(*itr);
    nsCOMPtr<mozIStorageError> error = bindingInternal->bind(aStatement);
    if (error) {
      
      mState = ERROR;

      
      (void)notifyError(error);
      return false;
    }

    
    itr++;
    bool lastStatement = aLastStatement && itr == end;
    continueProcessing = executeAndProcessStatement(aStatement, lastStatement);

    
    (void)::sqlite3_reset(aStatement);
  }

  return continueProcessing;
}

bool
AsyncExecuteStatements::executeAndProcessStatement(sqlite3_stmt *aStatement,
                                                   bool aLastStatement)
{
  mMutex.AssertNotCurrentThreadOwns();

  
  bool hasResults;
  do {
    hasResults = executeStatement(aStatement);

    
    if (mState == ERROR)
      return false;

    
    {
      MutexAutoLock lockedScope(mMutex);
      if (mCancelRequested) {
        mState = CANCELED;
        return false;
      }
    }

    
    
    if (mCallback && hasResults &&
        NS_FAILED(buildAndNotifyResults(aStatement))) {
      
      mState = ERROR;

      
      (void)notifyError(mozIStorageError::ERROR,
                        "An error occurred while notifying about results");

      return false;
    }
  } while (hasResults);

#ifdef DEBUG
  
  checkAndLogStatementPerformance(aStatement);
#endif

  
  
  
  if (aLastStatement)
    mState = COMPLETED;

  return true;
}

bool
AsyncExecuteStatements::executeStatement(sqlite3_stmt *aStatement)
{
  mMutex.AssertNotCurrentThreadOwns();
  Telemetry::AutoTimer<Telemetry::MOZ_STORAGE_ASYNC_REQUESTS_MS> finallySendExecutionDuration(mRequestStartDate);
  while (true) {
    
    SQLiteMutexAutoLock lockedScope(mDBMutex);

    int rc = mConnection->stepStatement(mNativeConnection, aStatement);
    
    if (rc == SQLITE_DONE)
    {
      Telemetry::Accumulate(Telemetry::MOZ_STORAGE_ASYNC_REQUESTS_SUCCESS, true);
      return false;
    }

    
    if (rc == SQLITE_ROW)
    {
      Telemetry::Accumulate(Telemetry::MOZ_STORAGE_ASYNC_REQUESTS_SUCCESS, true);
      return true;
    }

    
    if (rc == SQLITE_BUSY) {
      
      SQLiteMutexAutoUnlock unlockedScope(mDBMutex);

      
      (void)::PR_Sleep(PR_INTERVAL_NO_WAIT);
      continue;
    }

    
    mState = ERROR;
    Telemetry::Accumulate(Telemetry::MOZ_STORAGE_ASYNC_REQUESTS_SUCCESS, false);

    
    
    nsCOMPtr<mozIStorageError> errorObj(
      new Error(rc, ::sqlite3_errmsg(mNativeConnection))
    );
    
    SQLiteMutexAutoUnlock unlockedScope(mDBMutex);
    (void)notifyError(errorObj);

    
    return false;
  }
}

nsresult
AsyncExecuteStatements::buildAndNotifyResults(sqlite3_stmt *aStatement)
{
  NS_ASSERTION(mCallback, "Trying to dispatch results without a callback!");
  mMutex.AssertNotCurrentThreadOwns();

  
  if (!mResultSet)
    mResultSet = new ResultSet();
  NS_ENSURE_TRUE(mResultSet, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<Row> row(new Row());
  NS_ENSURE_TRUE(row, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = row->initialize(aStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mResultSet->add(row);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  TimeStamp now = TimeStamp::Now();
  TimeDuration delta = now - mIntervalStart;
  if (mResultSet->rows() >= MAX_ROWS_PER_RESULT || delta > mMaxWait) {
    
    rv = notifyResults();
    if (NS_FAILED(rv))
      return NS_OK; 

    
    mIntervalStart = now;
  }

  return NS_OK;
}

nsresult
AsyncExecuteStatements::notifyComplete()
{
  mMutex.AssertNotCurrentThreadOwns();
  NS_ASSERTION(mState != PENDING,
               "Still in a pending state when calling Complete!");

  
  
  
  for (uint32_t i = 0; i < mStatements.Length(); i++)
    mStatements[i].reset();

  
  
  
  
  mStatements.Clear();

  
  if (mHasTransaction) {
    if (mState == COMPLETED) {
      nsresult rv = mConnection->commitTransactionInternal(mNativeConnection);
      if (NS_FAILED(rv)) {
        mState = ERROR;
        (void)notifyError(mozIStorageError::ERROR,
                          "Transaction failed to commit");
      }
    }
    else {
      DebugOnly<nsresult> rv =
        mConnection->rollbackTransactionInternal(mNativeConnection);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Transaction failed to rollback");
    }
    mHasTransaction = false;
  }

  
  
  nsRefPtr<CompletionNotifier> completionEvent =
    new CompletionNotifier(mCallback, mState);

  
  mCallback = nullptr;

  (void)mCallingThread->Dispatch(completionEvent, NS_DISPATCH_NORMAL);

  return NS_OK;
}

nsresult
AsyncExecuteStatements::notifyError(int32_t aErrorCode,
                                    const char *aMessage)
{
  mMutex.AssertNotCurrentThreadOwns();
  mDBMutex.assertNotCurrentThreadOwns();

  if (!mCallback)
    return NS_OK;

  nsCOMPtr<mozIStorageError> errorObj(new Error(aErrorCode, aMessage));
  NS_ENSURE_TRUE(errorObj, NS_ERROR_OUT_OF_MEMORY);

  return notifyError(errorObj);
}

nsresult
AsyncExecuteStatements::notifyError(mozIStorageError *aError)
{
  mMutex.AssertNotCurrentThreadOwns();
  mDBMutex.assertNotCurrentThreadOwns();

  if (!mCallback)
    return NS_OK;

  nsRefPtr<ErrorNotifier> notifier =
    new ErrorNotifier(mCallback, aError, this);
  NS_ENSURE_TRUE(notifier, NS_ERROR_OUT_OF_MEMORY);

  return mCallingThread->Dispatch(notifier, NS_DISPATCH_NORMAL);
}

nsresult
AsyncExecuteStatements::notifyResults()
{
  mMutex.AssertNotCurrentThreadOwns();
  NS_ASSERTION(mCallback, "notifyResults called without a callback!");

  nsRefPtr<CallbackResultNotifier> notifier =
    new CallbackResultNotifier(mCallback, mResultSet, this);
  NS_ENSURE_TRUE(notifier, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mCallingThread->Dispatch(notifier, NS_DISPATCH_NORMAL);
  if (NS_SUCCEEDED(rv))
    mResultSet = nullptr; 
  return rv;
}

NS_IMPL_ISUPPORTS(
  AsyncExecuteStatements,
  nsIRunnable,
  mozIStoragePendingStatement
)

bool
AsyncExecuteStatements::statementsNeedTransaction()
{
  
  
  
  for (uint32_t i = 0, transactionsCount = 0; i < mStatements.Length(); ++i) {
    transactionsCount += mStatements[i].needsTransaction();
    if (transactionsCount > 1) {
      return true;
    }
  }
  return false;
}




NS_IMETHODIMP
AsyncExecuteStatements::Cancel()
{
#ifdef DEBUG
  bool onCallingThread = false;
  (void)mCallingThread->IsOnCurrentThread(&onCallingThread);
  NS_ASSERTION(onCallingThread, "Not canceling from the calling thread!");
#endif

  
  
  NS_ENSURE_FALSE(mCancelRequested, NS_ERROR_UNEXPECTED);

  {
    MutexAutoLock lockedScope(mMutex);

    
    mCancelRequested = true;
  }

  return NS_OK;
}




NS_IMETHODIMP
AsyncExecuteStatements::Run()
{
  MOZ_ASSERT(!mConnection->isClosed());

  
  {
    MutexAutoLock lockedScope(mMutex);
    if (mCancelRequested)
      mState = CANCELED;
  }
  if (mState == CANCELED)
    return notifyComplete();

  if (statementsNeedTransaction() && mConnection->getAutocommit()) {
    if (NS_SUCCEEDED(mConnection->beginTransactionInternal(mNativeConnection,
                                                           mozIStorageConnection::TRANSACTION_IMMEDIATE))) {
      mHasTransaction = true;
    }
#ifdef DEBUG
    else {
      NS_WARNING("Unable to create a transaction for async execution.");
    }
#endif
  }

  
  for (uint32_t i = 0; i < mStatements.Length(); i++) {
    bool finished = (i == (mStatements.Length() - 1));

    sqlite3_stmt *stmt;
    { 
      SQLiteMutexAutoLock lockedScope(mDBMutex);

      int rc = mStatements[i].getSqliteStatement(&stmt);
      if (rc != SQLITE_OK) {
        
        mState = ERROR;

        
        nsCOMPtr<mozIStorageError> errorObj(
          new Error(rc, ::sqlite3_errmsg(mNativeConnection))
        );
        {
          
          SQLiteMutexAutoUnlock unlockedScope(mDBMutex);
          (void)notifyError(errorObj);
        }
        break;
      }
    }

    
    if (mStatements[i].hasParametersToBeBound()) {
      if (!bindExecuteAndProcessStatement(mStatements[i], finished))
        break;
    }
    
    else if (!executeAndProcessStatement(stmt, finished)) {
      break;
    }
  }

  
  
  if (mResultSet)
    (void)notifyResults();

  
  return notifyComplete();
}

} 
} 
