






































#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsAutoLock.h"
#include "nsCOMArray.h"

#include "sqlite3.h"

#include "mozIStorageStatementCallback.h"
#include "mozIStoragePendingStatement.h"
#include "mozStorageResultSet.h"
#include "mozStorageRow.h"
#include "mozStorageBackground.h"
#include "mozStorageError.h"
#include "mozStorageEvents.h"







enum ExecutionState {
    PENDING = -1
  , COMPLETED = mozIStorageStatementCallback::REASON_FINISHED
  , CANCELED = mozIStorageStatementCallback::REASON_CANCELED
  , ERROR = mozIStorageStatementCallback::REASON_ERROR
};




class iCancelable : public nsISupports
{
public:
  


  virtual void cancel() = 0;
};




class iCompletionNotifier : public nsISupports
{
public:
  





  virtual void completed(iCancelable *aEvent) = 0;
};




class CallbackResultNotifier : public nsIRunnable
                             , public iCancelable
{
public:
  NS_DECL_ISUPPORTS

  CallbackResultNotifier(mozIStorageStatementCallback *aCallback,
                         mozIStorageResultSet *aResults,
                         iCompletionNotifier *aNotifier) :
      mCallback(aCallback)
    , mResults(aResults)
    , mCompletionNotifier(aNotifier)
    , mCanceled(PR_FALSE)
  {
  }

  NS_IMETHOD Run()
  {
    if (!mCanceled)
      (void)mCallback->HandleResult(mResults);

    
    mCompletionNotifier->completed(this);
    
    
    mCompletionNotifier = nsnull;
    return NS_OK;
  }

  virtual void cancel()
  {
    
    PR_AtomicSet(&mCanceled, PR_TRUE);
  }
private:
  CallbackResultNotifier() { }

  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<mozIStorageResultSet> mResults;
  nsRefPtr<iCompletionNotifier> mCompletionNotifier;
  PRInt32 mCanceled;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(
  CallbackResultNotifier,
  nsIRunnable
)




class ErrorNotifier : public nsIRunnable
                    , public iCancelable
{
public:
  NS_DECL_ISUPPORTS

  ErrorNotifier(mozIStorageStatementCallback *aCallback,
                mozIStorageError *aErrorObj,
                iCompletionNotifier *aCompletionNotifier) :
      mCallback(aCallback)
    , mErrorObj(aErrorObj)
    , mCanceled(PR_FALSE)
    , mCompletionNotifier(aCompletionNotifier)
  {
  }

  NS_IMETHOD Run()
  {
    if (!mCanceled && mCallback)
      (void)mCallback->HandleError(mErrorObj);

    mCompletionNotifier->completed(this);
    
    
    mCompletionNotifier = nsnull;
    return NS_OK;
  }

  virtual void cancel()
  {
    
    PR_AtomicSet(&mCanceled, PR_TRUE);
  }

  static inline iCancelable *Dispatch(nsIThread *aCallingThread,
                                      mozIStorageStatementCallback *aCallback,
                                      iCompletionNotifier *aCompletionNotifier,
                                      int aResult,
                                      const char *aMessage)
  {
    nsCOMPtr<mozIStorageError> errorObj(new mozStorageError(aResult, aMessage));
    if (!errorObj)
      return nsnull;

    ErrorNotifier *notifier =
      new ErrorNotifier(aCallback, errorObj, aCompletionNotifier);
    (void)aCallingThread->Dispatch(notifier, NS_DISPATCH_NORMAL);
    return notifier;
  }
private:
  ErrorNotifier() { }

  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<mozIStorageError> mErrorObj;
  PRInt32 mCanceled;
  nsRefPtr<iCompletionNotifier> mCompletionNotifier;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(
  ErrorNotifier,
  nsIRunnable
)




class CompletionNotifier : public nsIRunnable
                         , public iCancelable
{
public:
  NS_DECL_ISUPPORTS

  



  CompletionNotifier(mozIStorageStatementCallback *aCallback,
                     ExecutionState aReason,
                     iCompletionNotifier *aCompletionNotifier) :
      mCallback(aCallback)
    , mReason(aReason)
    , mCompletionNotifier(aCompletionNotifier)
  {
  }

  NS_IMETHOD Run()
  {
    (void)mCallback->HandleCompletion(mReason);
    NS_RELEASE(mCallback);

    mCompletionNotifier->completed(this);
    
    
    mCompletionNotifier = nsnull;
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
  nsRefPtr<iCompletionNotifier> mCompletionNotifier;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(
  CompletionNotifier,
  nsIRunnable
)




class AsyncExecute : public nsIRunnable
                   , public mozIStoragePendingStatement
                   , public iCompletionNotifier
{
public:
  NS_DECL_ISUPPORTS

  


  AsyncExecute(sqlite3_stmt *aStatement,
               mozIStorageStatementCallback *aCallback) :
      mStatement(aStatement)
    , mCallback(aCallback)
    , mCallingThread(do_GetCurrentThread())
    , mState(PENDING)
    , mStateMutex(nsAutoLock::NewLock("AsyncExecute::mStateMutex"))
    , mPendingEventsMutex(nsAutoLock::NewLock("AsyncExecute::mPendingEventsMutex"))
  {
  }

  nsresult initialize()
  {
    NS_ENSURE_TRUE(mStateMutex, NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(mPendingEventsMutex, NS_ERROR_OUT_OF_MEMORY);
    NS_IF_ADDREF(mCallback);
    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    
    {
      nsAutoLock mutex(mStateMutex);
      if (mState == CANCELED)
        return Complete();
    }

    
    
    nsresult rv = NS_OK;
    while (PR_TRUE) {
      int rc = sqlite3_step(mStatement);
      
      if (rc == SQLITE_DONE)
        break;

      
      if (rc != SQLITE_OK && rc != SQLITE_ROW) {
        if (rc == SQLITE_BUSY) {
          
          PR_Sleep(PR_INTERVAL_NO_WAIT);
          continue;
        }

        
        {
          nsAutoLock mutex(mStateMutex);
          mState = ERROR;
        }

        
        sqlite3 *db = sqlite3_db_handle(mStatement);
        iCancelable *cancelable = ErrorNotifier::Dispatch(
          mCallingThread, mCallback, this, rc, sqlite3_errmsg(db)
        );
        if (cancelable) {
          nsAutoLock mutex(mPendingEventsMutex);
          (void)mPendingEvents.AppendObject(cancelable);
        }

        
        return Complete();
      }

      
      {
        nsAutoLock mutex(mStateMutex);
        if (mState == CANCELED)
          return Complete();
      }

      
      
      if (!mCallback) {
        nsAutoLock mutex(mStateMutex);
        mState = COMPLETED;
        return Complete();
      }

      
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

      rv = row->initialize(mStatement);
      if (NS_FAILED(rv)) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      rv = results->add(row);
      if (NS_FAILED(rv))
        break;

      
      nsRefPtr<CallbackResultNotifier> notifier =
        new CallbackResultNotifier(mCallback, results, this);
      if (!notifier) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      nsresult status = mCallingThread->Dispatch(notifier, NS_DISPATCH_NORMAL);
      if (NS_SUCCEEDED(status)) {
        nsAutoLock mutex(mPendingEventsMutex);
        (void)mPendingEvents.AppendObject(notifier);
      }
    }

    
    
    if (NS_FAILED(rv)) {
      

      
      {
        nsAutoLock mutex(mStateMutex);
        mState = ERROR;
      }

      
      iCancelable *cancelable = ErrorNotifier::Dispatch(
        mCallingThread, mCallback, this, mozIStorageError::ERROR, ""
      );
      if (cancelable) {
        nsAutoLock mutex(mPendingEventsMutex);
        (void)mPendingEvents.AppendObject(cancelable);
      }
    }

    
    {
      nsAutoLock mutex(mStateMutex);
      if (mState == PENDING)
        mState = COMPLETED;

      
      return Complete();
    }
  }

  static PRBool cancelEnumerator(iCancelable *aCancelable, void *)
  {
    (void)aCancelable->cancel();
    return PR_TRUE;
  }

  NS_IMETHOD Cancel()
  {
    
    {
      nsAutoLock mutex(mStateMutex);
      NS_ENSURE_TRUE(mState == PENDING || mState == COMPLETED,
                     NS_ERROR_UNEXPECTED);
      mState = CANCELED;
    }

    
    {
      nsAutoLock mutex(mPendingEventsMutex);
      (void)mPendingEvents.EnumerateForwards(&AsyncExecute::cancelEnumerator,
                                             nsnull);
      mPendingEvents.Clear();
    }

    return NS_OK;
  }

  virtual void completed(iCancelable *aCancelable)
  {
    nsAutoLock mutex(mPendingEventsMutex);
    (void)mPendingEvents.RemoveObject(aCancelable);
  }

private:
  AsyncExecute() { }

  ~AsyncExecute()
  {
    NS_ASSERTION(mPendingEvents.Count() == 0, "Still pending events!");
    nsAutoLock::DestroyLock(mStateMutex);
    nsAutoLock::DestroyLock(mPendingEventsMutex);
  }

  



  nsresult Complete()
  {
    NS_ASSERTION(mState != PENDING,
                 "Still in a pending state when calling Complete!");

    
    (void)sqlite3_finalize(mStatement);
    mStatement = NULL;

    
    if (mCallback) {
      nsRefPtr<CompletionNotifier> completionEvent =
        new CompletionNotifier(mCallback, mState, this);
      nsresult rv = mCallingThread->Dispatch(completionEvent, NS_DISPATCH_NORMAL);
      if (NS_SUCCEEDED(rv)) {
        nsAutoLock mutex(mPendingEventsMutex);
        (void)mPendingEvents.AppendObject(completionEvent);
      }

      
      mCallback = nsnull;
    }

    return NS_OK;
  }

  sqlite3_stmt *mStatement;
  mozIStorageStatementCallback *mCallback;
  nsCOMPtr<nsIThread> mCallingThread;

  


  ExecutionState mState;

  


  PRLock *mStateMutex;

  



  nsCOMArray<iCancelable> mPendingEvents;

  


  PRLock *mPendingEventsMutex;
};
NS_IMPL_THREADSAFE_ISUPPORTS2(
  AsyncExecute,
  nsIRunnable,
  mozIStoragePendingStatement
)

nsresult
NS_executeAsync(sqlite3_stmt *aStatement,
                mozIStorageStatementCallback *aCallback,
                mozIStoragePendingStatement **_stmt)
{
  
  nsRefPtr<AsyncExecute> event(new AsyncExecute(aStatement, aCallback));
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = event->initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsIEventTarget *target = mozStorageBackground::getService()->target();
  rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ADDREF(*_stmt = event);
  return NS_OK;
}
