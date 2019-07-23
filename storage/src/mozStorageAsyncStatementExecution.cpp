






































#include "nsAutoPtr.h"
#include "prtime.h"

#include "sqlite3.h"

#include "mozIStorageStatementCallback.h"
#include "mozStorageHelper.h"
#include "mozStorageResultSet.h"
#include "mozStorageRow.h"
#include "mozStorageConnection.h"
#include "mozStorageError.h"
#include "mozStoragePrivateHelpers.h"
#include "mozStorageAsyncStatementExecution.h"

namespace mozilla {
namespace storage {












#define MAX_MILLISECONDS_BETWEEN_RESULTS 100
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
AsyncExecuteStatements::execute(sqlite3_stmt_array &aStatements,
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

AsyncExecuteStatements::AsyncExecuteStatements(sqlite3_stmt_array &aStatements,
                                               Connection *aConnection,
                                               mozIStorageStatementCallback *aCallback)
: mConnection(aConnection)
, mTransactionManager(nsnull)
, mCallback(aCallback)
, mCallingThread(::do_GetCurrentThread())
, mMaxIntervalWait(::PR_MicrosecondsToInterval(MAX_MILLISECONDS_BETWEEN_RESULTS))
, mIntervalStart(::PR_IntervalNow())
, mState(PENDING)
, mCancelRequested(PR_FALSE)
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
  PRBool onCallingThread = PR_FALSE;
  (void)mCallingThread->IsOnCurrentThread(&onCallingThread);
  NS_ASSERTION(onCallingThread, "runEvent not running on the calling thread!");
#endif

  
  
  
  return !mCancelRequested;
}

bool
AsyncExecuteStatements::executeAndProcessStatement(sqlite3_stmt *aStatement,
                                                   bool aLastStatement)
{
  mMutex.AssertNotCurrentThreadOwns();

  
  
  
  MutexAutoLock lockedScope(mMutex);

  
  bool hasResults = executeStatement(aStatement);

  
  if (mState == ERROR)
    return false;

  
  if (mCancelRequested) {
    mState = CANCELED;
    return false;
  }

  
  
  if (mCallback && hasResults && NS_FAILED(buildAndNotifyResults(aStatement))) {
    
    mState = ERROR;

    {
      
      MutexAutoUnlock unlockedScope(mMutex);

      
      (void)notifyError(mozIStorageError::ERROR,
                        "An error occurred while notifying about results");
    }

    return false;
  }

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
  mMutex.AssertCurrentThreadOwns();

  while (true) {
    int rc = ::sqlite3_step(aStatement);
    
    if (rc == SQLITE_DONE)
      return false;

    
    if (rc == SQLITE_ROW)
      return true;

    
    if (rc == SQLITE_BUSY) {
      
      MutexAutoUnlock cancelationScope(mMutex);

      
      (void)::PR_Sleep(PR_INTERVAL_NO_WAIT);
      continue;
    }

    
    mState = ERROR;

    {
      
      MutexAutoUnlock unlockedScope(mMutex);

      
      sqlite3 *db = ::sqlite3_db_handle(aStatement);
      (void)notifyError(rc, ::sqlite3_errmsg(db));
    }

    
    return false;
  }
}

nsresult
AsyncExecuteStatements::buildAndNotifyResults(sqlite3_stmt *aStatement)
{
  NS_ASSERTION(mCallback, "Trying to dispatch results without a callback!");
  mMutex.AssertCurrentThreadOwns();

  
  
  
  MutexAutoUnlock cancelationScope(mMutex);

  
  if (!mResultSet)
    mResultSet = new ResultSet();
  NS_ENSURE_TRUE(mResultSet, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<Row> row(new Row());
  NS_ENSURE_TRUE(row, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = row->initialize(aStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mResultSet->add(row);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  PRIntervalTime now = ::PR_IntervalNow();
  PRIntervalTime delta = now - mIntervalStart;
  if (mResultSet->rows() >= MAX_ROWS_PER_RESULT || delta > mMaxIntervalWait) {
    
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

  
  
  
  for (PRUint32 i = 0; i < mStatements.Length(); i++) {
    (void)::sqlite3_finalize(mStatements[i]);
    mStatements[i] = NULL;
  }

  
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

  nsRefPtr<ErrorNotifier> notifier =
    new ErrorNotifier(mCallback, errorObj, this);
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
AsyncExecuteStatements::Cancel(PRBool *_successful)
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

    
    *_successful = (mState == PENDING);
  }

  
  
  
  
  

  return NS_OK;
}




NS_IMETHODIMP
AsyncExecuteStatements::Run()
{
  
  bool cancelRequested;
  {
    MutexAutoLock lockedScope(mMutex);
    cancelRequested = mCancelRequested;
    if (cancelRequested)
      mState = CANCELED;
  }
  if (cancelRequested)
    return notifyComplete();

  
  
  
  if (mStatements.Length() > 1) {
    
    mTransactionManager = new mozStorageTransaction(mConnection, PR_FALSE,
                                                    mozIStorageConnection::TRANSACTION_IMMEDIATE);
  }

  
  for (PRUint32 i = 0; i < mStatements.Length(); i++) {
    PRBool finished = (i == (mStatements.Length() - 1));
    if (!executeAndProcessStatement(mStatements[i], finished))
      break;
  }

  
  
  if (mResultSet)
    (void)notifyResults();

  
  return notifyComplete();
}

} 
} 
