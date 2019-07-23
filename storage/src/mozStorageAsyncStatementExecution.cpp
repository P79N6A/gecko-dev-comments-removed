






































#include "nsAutoPtr.h"
#include "prtime.h"

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

namespace mozilla {
namespace storage {












#define MAX_MILLISECONDS_BETWEEN_RESULTS 75
#define MAX_ROWS_PER_RESULT 15




namespace {

typedef AsyncExecuteStatements::ExecutionState ExecutionState;




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

    if (mEventStatus->shouldNotify())
      (void)mCallback->HandleResult(mResults);

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
    if (mEventStatus->shouldNotify() && mCallback)
      (void)mCallback->HandleError(mErrorObj);

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
                     ExecutionState aReason) :
      mCallback(aCallback)
    , mReason(aReason)
  {
  }

  NS_IMETHOD Run()
  {
    (void)mCallback->HandleCompletion(mReason);
    NS_RELEASE(mCallback);

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
                                mozIStorageStatementCallback *aCallback,
                                mozIStoragePendingStatement **_stmt)
{
  
  nsRefPtr<AsyncExecuteStatements> event =
    new AsyncExecuteStatements(aStatements, aConnection, aCallback);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  
  nsCOMPtr<nsIEventTarget> target(aConnection->getAsyncExecutionTarget());
  NS_ENSURE_TRUE(target, NS_ERROR_NOT_AVAILABLE);
  nsresult rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ADDREF(*_stmt = event);
  return NS_OK;
}

AsyncExecuteStatements::AsyncExecuteStatements(StatementDataArray &aStatements,
                                               Connection *aConnection,
                                               mozIStorageStatementCallback *aCallback)
: mConnection(aConnection)
, mTransactionManager(nsnull)
, mCallback(aCallback)
, mCallingThread(::do_GetCurrentThread())
, mMaxWait(TimeDuration::FromMilliseconds(MAX_MILLISECONDS_BETWEEN_RESULTS))
, mIntervalStart(TimeStamp::Now())
, mState(PENDING)
, mCancelRequested(false)
, mMutex(aConnection->sharedAsyncExecutionMutex)
{
  (void)mStatements.SwapElements(aStatements);
  NS_ASSERTION(mStatements.Length(), "We weren't given any statements!");
  NS_IF_ADDREF(mCallback);
}

bool
AsyncExecuteStatements::shouldNotify()
{
#ifdef DEBUG
  mMutex.AssertNotCurrentThreadOwns();

  PRBool onCallingThread = PR_FALSE;
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

  sqlite3_stmt *stmt(aData);
  BindingParamsArray *paramsArray(aData);

  
  bool continueProcessing = true;
  BindingParamsArray::iterator itr = paramsArray->begin();
  BindingParamsArray::iterator end = paramsArray->end();
  while (itr != end && continueProcessing) {
    
    nsCOMPtr<mozIStorageError> error;
    error = (*itr)->bind(stmt);
    if (error) {
      
      mState = ERROR;

      
      (void)notifyError(error);
      return false;
    }

    
    itr++;
    bool lastStatement = aLastStatement && itr == end;
    continueProcessing = executeAndProcessStatement(stmt, lastStatement);

    
    (void)::sqlite3_reset(stmt);
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

  while (true) {
    int rc = ::sqlite3_step(aStatement);
    
    if (rc == SQLITE_DONE)
      return false;

    
    if (rc == SQLITE_ROW)
      return true;

    
    if (rc == SQLITE_BUSY) {
      
      (void)::PR_Sleep(PR_INTERVAL_NO_WAIT);
      continue;
    }

    
    mState = ERROR;

    
    sqlite3 *db = ::sqlite3_db_handle(aStatement);
    (void)notifyError(rc, ::sqlite3_errmsg(db));

    
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

  
  
  
  for (PRUint32 i = 0; i < mStatements.Length(); i++)
    mStatements[i].finalize();

  
  if (mTransactionManager) {
    if (mState == COMPLETED) {
      nsresult rv = mTransactionManager->Commit();
      if (NS_FAILED(rv)) {
        mState = ERROR;
        (void)notifyError(mozIStorageError::ERROR,
                          "Transaction failed to commit");
      }
    }
    else {
      (void)mTransactionManager->Rollback();
    }
    delete mTransactionManager;
    mTransactionManager = nsnull;
  }

  
  if (mCallback) {
    nsRefPtr<CompletionNotifier> completionEvent =
      new CompletionNotifier(mCallback, mState);
    NS_ENSURE_TRUE(completionEvent, NS_ERROR_OUT_OF_MEMORY);

    
    mCallback = nsnull;

    (void)mCallingThread->Dispatch(completionEvent, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
}

nsresult
AsyncExecuteStatements::notifyError(PRInt32 aErrorCode,
                                    const char *aMessage)
{
  mMutex.AssertNotCurrentThreadOwns();

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
    mResultSet = nsnull; 
  return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(
  AsyncExecuteStatements,
  nsIRunnable,
  mozIStoragePendingStatement
)




NS_IMETHODIMP
AsyncExecuteStatements::Cancel()
{
#ifdef DEBUG
  PRBool onCallingThread = PR_FALSE;
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
  
  {
    MutexAutoLock lockedScope(mMutex);
    if (mCancelRequested)
      mState = CANCELED;
  }
  if (mState == CANCELED)
    return notifyComplete();

  
  
  
  
  
  if (mStatements.Length() > 1 || mStatements[0].needsTransaction()) {
    
    mTransactionManager = new mozStorageTransaction(mConnection, PR_FALSE,
                                                    mozIStorageConnection::TRANSACTION_IMMEDIATE);
  }

  
  for (PRUint32 i = 0; i < mStatements.Length(); i++) {
    bool finished = (i == (mStatements.Length() - 1));

    
    if (mStatements[i].hasParametersToBeBound()) {
      if (!bindExecuteAndProcessStatement(mStatements[i], finished))
        break;
    }
    
    else if (!executeAndProcessStatement(mStatements[i], finished)) {
      break;
    }
  }

  
  
  if (mResultSet)
    (void)notifyResults();

  
  return notifyComplete();
}

} 
} 
