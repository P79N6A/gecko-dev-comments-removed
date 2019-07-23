






































#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsAutoLock.h"
#include "nsCOMArray.h"

#include "sqlite3.h"

#include "mozIStorageStatementCallback.h"
#include "mozIStoragePendingStatement.h"
#include "mozStorageHelper.h"
#include "mozStorageResultSet.h"
#include "mozStorageRow.h"
#include "mozStorageConnection.h"
#include "mozStorageError.h"
#include "mozStorageEvents.h"







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




class AsyncExecute : public nsIRunnable
                   , public mozIStoragePendingStatement
                   , public iEventStatus
{
public:
  NS_DECL_ISUPPORTS

  


  AsyncExecute(nsTArray<sqlite3_stmt *> &aStatements,
               mozIStorageConnection *aConnection,
               mozIStorageStatementCallback *aCallback) :
      mConnection(aConnection)
    , mTransactionManager(nsnull)
    , mCallback(aCallback)
    , mCallingThread(do_GetCurrentThread())
    , mState(PENDING)
    , mCancelRequested(PR_FALSE)
    , mLock(nsAutoLock::NewLock("AsyncExecute::mLock"))
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

    
    nsresult rv = NS_OK;
    for (PRUint32 i = 0; i < mStatements.Length(); i++) {
      
      
      
      nsAutoLock mutex(mLock);

      while (PR_TRUE) {
        int rc = sqlite3_step(mStatements[i]);
        
        if (rc == SQLITE_DONE)
          break;

        
        if (rc != SQLITE_OK && rc != SQLITE_ROW) {
          if (rc == SQLITE_BUSY) {
            
            nsAutoUnlock cancelationScope(mLock);

            
            PR_Sleep(PR_INTERVAL_NO_WAIT);
            continue;
          }

          
          mState = ERROR;

          
          mutex.unlock();

          
          sqlite3 *db = sqlite3_db_handle(mStatements[i]);
          (void)NotifyError(rc, sqlite3_errmsg(db));

          
          return NotifyComplete();
        }

        
        
        if (!mCallback)
          break;

        
        if (mCancelRequested) {
          mState = CANCELED;
          mutex.unlock();
          return NotifyComplete();
        }

        
        
        
        
        nsAutoUnlock cancelationScope(mLock);

        
        
        nsRefPtr<mozStorageResultSet> results(new mozStorageResultSet());
        if (!results) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        nsRefPtr<mozStorageRow> row(new mozStorageRow());
        if (!row) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        rv = row->initialize(mStatements[i]);
        if (NS_FAILED(rv))
          break;

        rv = results->add(row);
        if (NS_FAILED(rv))
          break;

        
        (void)NotifyResults(results);
      }

      
      
      if (NS_FAILED(rv)) {
        mState = ERROR;

        
        mutex.unlock();
        (void)NotifyError(mozIStorageError::ERROR, "");
        break;
      }

      
      
      
      if (i == (mStatements.Length() - 1))
        mState = COMPLETED;
    }

    
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
  AsyncExecute() { }

  ~AsyncExecute()
  {
    nsAutoLock::DestroyLock(mLock);
  }

  


  nsresult NotifyComplete()
  {
    NS_ASSERTION(mState != PENDING,
                 "Still in a pending state when calling Complete!");

    
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

    
    for (PRUint32 i = 0; i < mStatements.Length(); i++) {
      (void)sqlite3_finalize(mStatements[i]);
      mStatements[i] = NULL;
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

  





  nsresult NotifyResults(mozStorageResultSet *aResultSet)
  {
    NS_ASSERTION(mCallback, "NotifyResults called without a callback!");

    nsRefPtr<CallbackResultNotifier> notifier =
      new CallbackResultNotifier(mCallback, aResultSet, this);
    NS_ENSURE_TRUE(notifier, NS_ERROR_OUT_OF_MEMORY);

    return mCallingThread->Dispatch(notifier, NS_DISPATCH_NORMAL);
  };

  nsTArray<sqlite3_stmt *> mStatements;
  mozIStorageConnection *mConnection;
  mozStorageTransaction *mTransactionManager;
  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<nsIThread> mCallingThread;

  


  ExecutionState mState;

  


  PRBool mCancelRequested;

  







  PRLock *mLock;
};
NS_IMPL_THREADSAFE_ISUPPORTS2(
  AsyncExecute,
  nsIRunnable,
  mozIStoragePendingStatement
)

nsresult
NS_executeAsync(nsTArray<sqlite3_stmt *> &aStatements,
                mozStorageConnection *aConnection,
                mozIStorageStatementCallback *aCallback,
                mozIStoragePendingStatement **_stmt)
{
  
  nsRefPtr<AsyncExecute> event(new AsyncExecute(aStatements, aConnection, aCallback));
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
