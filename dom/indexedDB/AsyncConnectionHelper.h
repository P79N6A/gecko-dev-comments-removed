






































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

  





  static const PRUint16 OK = PR_UINT16_MAX;

  





  static const PRUint16 NOREPLY = OK - 1;

  nsresult Dispatch(nsIEventTarget* aDatabaseThread);

  
  nsresult DispatchToTransactionPool();

  void SetError(PRUint16 aErrorCode)
  {
    mError = true;
    mErrorCode = aErrorCode;
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

  



  virtual PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection) = 0;

  






  virtual PRUint16 OnSuccess(nsIDOMEventTarget* aTarget);

  




  virtual void OnError(nsIDOMEventTarget* aTarget,
                       PRUint16 aErrorCode);

  






  virtual PRUint16 GetSuccessResult(nsIWritableVariant* aVariant);

protected:
  nsRefPtr<IDBDatabase> mDatabase;
  nsRefPtr<IDBTransaction> mTransaction;
  nsRefPtr<IDBRequest> mRequest;

private:
  nsCOMPtr<mozIStorageProgressHandler> mOldProgressHandler;

  mozilla::TimeStamp mStartTime;
  mozilla::TimeDuration mTimeoutDuration;

  PRUint16 mErrorCode;
  PRPackedBool mError;
};

END_INDEXEDDB_NAMESPACE

#endif 
