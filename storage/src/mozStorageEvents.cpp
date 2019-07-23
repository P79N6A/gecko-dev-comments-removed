






































#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsAutoLock.h"
#include "nsCOMArray.h"
#include "prtime.h"

#include "sqlite3.h"

#include "mozIStorageStatementCallback.h"
#include "mozIStoragePendingStatement.h"
#include "mozStorageHelper.h"
#include "mozStorageResultSet.h"
#include "mozStorageRow.h"
#include "mozStorageConnection.h"
#include "mozStorageError.h"
#include "mozStoragePrivateHelpers.h"
#include "mozStorageEvents.h"












#define MAX_MILLISECONDS_BETWEEN_RESULTS 100
#define MAX_ROWS_PER_RESULT 15







enum ExecutionState {
    PENDING = -1
  , COMPLETED = mozIStorageStatementCallback::REASON_FINISHED
  , CANCELED = mozIStorageStatementCallback::REASON_CANCELED
  , ERROR = mozIStorageStatementCallback::REASON_ERROR
};




class iEventStatus : public nsISupports
{
public:
  virtual PRBool runEvent() = 0;
};




class CallbackResultNotifier : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  CallbackResultNotifier(mozIStorageStatementCallback *aCallback,
                         mozIStorageResultSet *aResults,
                         iEventStatus *aEventStatus) :
      mCallback(aCallback)
    , mResults(aResults)
    , mEventStatus(aEventStatus)
  {
  }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(mCallback, "Trying to notify about results without a callback!");

    if (mEventStatus->runEvent())
      (void)mCallback->HandleResult(mResults);

    return NS_OK;
  }

private:
  CallbackResultNotifier() { }

  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<mozIStorageResultSet> mResults;
  nsRefPtr<iEventStatus> mEventStatus;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(
  CallbackResultNotifier,
  nsIRunnable
)




class ErrorNotifier : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  ErrorNotifier(mozIStorageStatementCallback *aCallback,
                mozIStorageError *aErrorObj,
                iEventStatus *aEventStatus) :
      mCallback(aCallback)
    , mErrorObj(aErrorObj)
    , mEventStatus(aEventStatus)
  {
  }

  NS_IMETHOD Run()
  {
    if (mEventStatus->runEvent() && mCallback)
      (void)mCallback->HandleError(mErrorObj);

    return NS_OK;
  }

private:
  ErrorNotifier() { }

  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<mozIStorageError> mErrorObj;
  nsRefPtr<iEventStatus> mEventStatus;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(
  ErrorNotifier,
  nsIRunnable
)




class CompletionNotifier : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  



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

  virtual void cancel()
  {
    
    mReason = CANCELED;
  }

private:
  CompletionNotifier() { }

  mozIStorageStatementCallback *mCallback;
  ExecutionState mReason;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(
  CompletionNotifier,
  nsIRunnable
)




class AsyncExecuteStatements : public nsIRunnable
                             , public mozIStoragePendingStatement
                             , public iEventStatus
{
public:
  NS_DECL_ISUPPORTS

  


  AsyncExecuteStatements(nsTArray<sqlite3_stmt *> &aStatements,
                         mozIStorageConnection *aConnection,
                         mozIStorageStatementCallback *aCallback) :
      mConnection(aConnection)
    , mTransactionManager(nsnull)
    , mCallback(aCallback)
    , mCallingThread(do_GetCurrentThread())
    , mMaxIntervalWait(PR_MicrosecondsToInterval(MAX_MILLISECONDS_BETWEEN_RESULTS))
    , mIntervalStart(PR_IntervalNow())
    , mState(PENDING)
    , mCancelRequested(PR_FALSE)
    , mLock(nsAutoLock::NewLock("AsyncExecuteStatements::mLock"))
  {
    (void)mStatements.SwapElements(aStatements);
    NS_ASSERTION(mStatements.Length(), "We weren't given any statements!");
  }

  nsresult initialize()
  {
    NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);
    NS_IF_ADDREF(mCallback);
    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    
    {
      nsAutoLock mutex(mLock);
      if (mCancelRequested) {
        mState = CANCELED;
        mutex.unlock();
        return NotifyComplete();
      }
    }

    
    
    
    if (mStatements.Length() > 1) {
      
      mTransactionManager = new mozStorageTransaction(mConnection, PR_FALSE,
                                                      mozIStorageConnection::TRANSACTION_IMMEDIATE);
    }

    
    for (PRUint32 i = 0; i < mStatements.Length(); i++) {
      PRBool finished = (i == (mStatements.Length() - 1));
      if (!ExecuteAndProcessStatement(mStatements[i], finished))
        break;
    }

    
    
    if (mResultSet)
      (void)NotifyResults();

    
    return NotifyComplete();
  }

  NS_IMETHOD Cancel(PRBool *_successful)
  {
#ifdef DEBUG
    PRBool onCallingThread = PR_FALSE;
    (void)mCallingThread->IsOnCurrentThread(&onCallingThread);
    NS_ASSERTION(onCallingThread, "Not canceling from the calling thread!");
#endif

    
    
    NS_ENSURE_FALSE(mCancelRequested, NS_ERROR_UNEXPECTED);

    {
      nsAutoLock mutex(mLock);

      
      mCancelRequested = PR_TRUE;

      
      *_successful = (mState == PENDING);
    }

    
    
    
    
    

    return NS_OK;
  }

  



  PRBool runEvent()
  {
#ifdef DEBUG
    PRBool onCallingThread = PR_FALSE;
    (void)mCallingThread->IsOnCurrentThread(&onCallingThread);
    NS_ASSERTION(onCallingThread, "runEvent not running on the calling thread!");
#endif

    
    
    
    return !mCancelRequested;
  }

private:
  ~AsyncExecuteStatements()
  {
    nsAutoLock::DestroyLock(mLock);
  }

  













  PRBool ExecuteAndProcessStatement(sqlite3_stmt *aStatement, PRBool aFinished)
  {
    
    
    
    nsAutoLock mutex(mLock);

    nsresult rv = NS_OK;
    while (PR_TRUE) {
      int rc = sqlite3_step(aStatement);
      
      if (rc == SQLITE_DONE)
        break;

      
      if (rc != SQLITE_OK && rc != SQLITE_ROW) {
        if (rc == SQLITE_BUSY) {
          
          nsAutoUnlock cancelationScope(mLock);

          
          (void)PR_Sleep(PR_INTERVAL_NO_WAIT);
          continue;
        }

        
        mState = ERROR;

        
        mutex.unlock();

        
        sqlite3 *db = sqlite3_db_handle(aStatement);
        (void)NotifyError(rc, sqlite3_errmsg(db));

        
        return PR_FALSE;
      }

      
      
      
      
      if (!mCallback)
        break;

      
      if (mCancelRequested) {
        mState = CANCELED;
        return PR_FALSE;
      }

      
      rv = BuildAndNotifyResults(aStatement);
      if (NS_FAILED(rv))
        break;
    }

    
    
    if (NS_FAILED(rv)) {
      mState = ERROR;

      
      mutex.unlock();

      
      (void)NotifyError(mozIStorageError::ERROR, "");
      return PR_FALSE;
    }

#ifdef DEBUG
    
    CheckAndLogStatementPerformance(aStatement);
#endif

    
    
    
    if (aFinished)
      mState = COMPLETED;

    return PR_TRUE;
  }

  








  nsresult BuildAndNotifyResults(sqlite3_stmt *aStatement)
  {
    NS_ASSERTION(mCallback, "Trying to dispatch results without a callback!");

    
    
    
    nsAutoUnlock cancelationScope(mLock);

    
    if (!mResultSet)
      mResultSet = new mozStorageResultSet();
    NS_ENSURE_TRUE(mResultSet, NS_ERROR_OUT_OF_MEMORY);

    nsRefPtr<mozStorageRow> row(new mozStorageRow());
    NS_ENSURE_TRUE(row, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = row->initialize(aStatement);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mResultSet->add(row);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    PRIntervalTime now = PR_IntervalNow();
    PRIntervalTime delta = now - mIntervalStart;
    if (mResultSet->rows() >= MAX_ROWS_PER_RESULT || delta > mMaxIntervalWait) {
      
      rv = NotifyResults();
      if (NS_FAILED(rv))
        return NS_OK; 

      
      mIntervalStart = now;
    }

    return NS_OK;
  }

  




  nsresult NotifyComplete()
  {
    NS_ASSERTION(mState != PENDING,
                 "Still in a pending state when calling Complete!");

    
    
    
    for (PRUint32 i = 0; i < mStatements.Length(); i++) {
      (void)sqlite3_finalize(mStatements[i]);
      mStatements[i] = NULL;
    }

    
    if (mTransactionManager) {
      if (mState == COMPLETED) {
        nsresult rv = mTransactionManager->Commit();
        if (NS_FAILED(rv)) {
          mState = ERROR;
          (void)NotifyError(mozIStorageError::ERROR,
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

  









  nsresult NotifyError(PRInt32 aErrorCode, const char *aMessage)
  {
    if (!mCallback)
      return NS_OK;

    nsCOMPtr<mozIStorageError> errorObj =
      new mozStorageError(aErrorCode, aMessage);
    NS_ENSURE_TRUE(errorObj, NS_ERROR_OUT_OF_MEMORY);

    nsRefPtr<ErrorNotifier> notifier =
      new ErrorNotifier(mCallback, errorObj, this);
    NS_ENSURE_TRUE(notifier, NS_ERROR_OUT_OF_MEMORY);

    return mCallingThread->Dispatch(notifier, NS_DISPATCH_NORMAL);
  }

  




  nsresult NotifyResults()
  {
    NS_ASSERTION(mCallback, "NotifyResults called without a callback!");

    nsRefPtr<CallbackResultNotifier> notifier =
      new CallbackResultNotifier(mCallback, mResultSet, this);
    NS_ENSURE_TRUE(notifier, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = mCallingThread->Dispatch(notifier, NS_DISPATCH_NORMAL);
    if (NS_SUCCEEDED(rv))
      mResultSet = nsnull; 
    return rv;
  };

  nsTArray<sqlite3_stmt *> mStatements;
  mozIStorageConnection *mConnection;
  mozStorageTransaction *mTransactionManager;
  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<nsIThread> mCallingThread;
  nsRefPtr<mozStorageResultSet> mResultSet;

  



  const PRIntervalTime mMaxIntervalWait;

  


  PRIntervalTime mIntervalStart;

  


  ExecutionState mState;

  


  PRBool mCancelRequested;

  







  PRLock *mLock;
};
NS_IMPL_THREADSAFE_ISUPPORTS2(
  AsyncExecuteStatements,
  nsIRunnable,
  mozIStoragePendingStatement
)

nsresult
NS_executeAsync(nsTArray<sqlite3_stmt *> &aStatements,
                mozStorageConnection *aConnection,
                mozIStorageStatementCallback *aCallback,
                mozIStoragePendingStatement **_stmt)
{
  
  nsRefPtr<AsyncExecuteStatements> event =
    new AsyncExecuteStatements(aStatements, aConnection, aCallback);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = event->initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIEventTarget> target(aConnection->getAsyncExecutionTarget());
  NS_ENSURE_TRUE(target, NS_ERROR_NOT_AVAILABLE);
  rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ADDREF(*_stmt = event);
  return NS_OK;
}
