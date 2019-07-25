






































#ifndef mozilla_dom_indexeddb_asyncconnectionhelper_h__
#define mozilla_dom_indexeddb_asyncconnectionhelper_h__


#include "IndexedDatabase.h"
#include "IDBDatabase.h"
#include "IDBRequest.h"

#include "mozIStorageProgressHandler.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIVariant.h"

#include "mozilla/TimeStamp.h"

class mozIStorageConnection;

BEGIN_INDEXEDDB_NAMESPACE

class IDBTransaction;










class AsyncConnectionHelper : public nsIRunnable,
                              public mozIStorageProgressHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_MOZISTORAGEPROGRESSHANDLER

  nsresult Dispatch(nsIEventTarget* aDatabaseThread);

  
  nsresult DispatchToTransactionPool();

  void SetError(nsresult aErrorCode)
  {
    NS_ASSERTION(NS_FAILED(aErrorCode), "Not a failure code!");
    mResultCode = aErrorCode;
  }

  static IDBTransaction* GetCurrentTransaction();

  nsISupports* GetSource()
  {
    return mRequest ? mRequest->Source() : nsnull;
  }

protected:
  AsyncConnectionHelper(IDBDatabase* aDatabase,
                        IDBRequest* aRequest);

  AsyncConnectionHelper(IDBTransaction* aTransaction,
                        IDBRequest* aRequest);

  virtual ~AsyncConnectionHelper();

  


  void SetTimeoutMS(PRUint32 aTimeoutMS)
  {
    mTimeoutDuration = TimeDuration::FromMilliseconds(aTimeoutMS);
  }

  





  virtual nsresult Init();

  



  virtual nsresult DoDatabaseWork(mozIStorageConnection* aConnection) = 0;

  






  virtual nsresult OnSuccess(nsIDOMEventTarget* aTarget);

  




  virtual void OnError(nsIDOMEventTarget* aTarget,
                       nsresult aErrorCode);

  






  virtual nsresult GetSuccessResult(nsIWritableVariant* aVariant);

  




  virtual void ReleaseMainThreadObjects();

protected:
  nsRefPtr<IDBDatabase> mDatabase;
  nsRefPtr<IDBTransaction> mTransaction;
  nsRefPtr<IDBRequest> mRequest;

private:
  nsCOMPtr<mozIStorageProgressHandler> mOldProgressHandler;

  mozilla::TimeStamp mStartTime;
  mozilla::TimeDuration mTimeoutDuration;

  nsresult mResultCode;
  PRPackedBool mDispatched;
};

END_INDEXEDDB_NAMESPACE

#endif 
