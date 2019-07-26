





#ifndef mozilla_dom_indexeddb_asyncconnectionhelper_h__
#define mozilla_dom_indexeddb_asyncconnectionhelper_h__


#include "DatabaseInfo.h"
#include "IndexedDatabase.h"
#include "IDBDatabase.h"
#include "IDBRequest.h"

#include "mozIStorageProgressHandler.h"
#include "nsIEventTarget.h"
#include "nsIRunnable.h"

#include "nsDOMEvent.h"

class mozIStorageConnection;

BEGIN_INDEXEDDB_NAMESPACE

class AutoSetCurrentTransaction;
class IDBTransaction;

namespace ipc {
class ResponseValue;
}



class HelperBase : public nsIRunnable
{
  friend class IDBRequest;

public:

  virtual nsresult GetResultCode() = 0;

  virtual nsresult GetSuccessResult(JSContext* aCx,
                                    jsval* aVal) = 0;

  IDBRequest* GetRequest() const
  {
    return mRequest;
  }

protected:
  HelperBase(IDBRequest* aRequest)
    : mRequest(aRequest)
  { }

  virtual ~HelperBase();

  



  nsresult WrapNative(JSContext* aCx,
                      nsISupports* aNative,
                      jsval* aResult);

  




  virtual void ReleaseMainThreadObjects();

  nsRefPtr<IDBRequest> mRequest;
};










class AsyncConnectionHelper : public HelperBase,
                              public mozIStorageProgressHandler
{
  friend class AutoSetCurrentTransaction;

public:
  typedef ipc::ResponseValue ResponseValue;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_MOZISTORAGEPROGRESSHANDLER

  virtual nsresult Dispatch(nsIEventTarget* aDatabaseThread);

  
  nsresult DispatchToTransactionPool();

  void SetError(nsresult aErrorCode)
  {
    NS_ASSERTION(NS_FAILED(aErrorCode), "Not a failure code!");
    mResultCode = aErrorCode;
  }

  static IDBTransaction* GetCurrentTransaction();

  bool HasTransaction() const
  {
    return !!mTransaction;
  }

  IDBTransaction* GetTransaction() const
  {
    return mTransaction;
  }

  nsISupports* GetSource() const
  {
    return mRequest ? mRequest->Source() : nullptr;
  }

  virtual nsresult GetResultCode() MOZ_OVERRIDE
  {
    return mResultCode;
  }

  enum ChildProcessSendResult
  {
    
    Success_Sent = 0,

    
    Success_NotSent,

    
    
    Success_ActorDisconnected,

    
    Error
  };

  ChildProcessSendResult
  MaybeSendResponseToChildProcess(nsresult aResultCode);

  virtual nsresult OnParentProcessRequestComplete(
                                           const ResponseValue& aResponseValue);

  virtual nsresult
  UnpackResponseFromParentProcess(const ResponseValue& aResponseValue) = 0;

protected:
  AsyncConnectionHelper(IDBDatabase* aDatabase,
                        IDBRequest* aRequest);

  AsyncConnectionHelper(IDBTransaction* aTransaction,
                        IDBRequest* aRequest);

  virtual ~AsyncConnectionHelper();

  




  virtual nsresult Init();

  


  virtual nsresult DoDatabaseWork(mozIStorageConnection* aConnection) = 0;

  




  virtual already_AddRefed<nsIDOMEvent> CreateSuccessEvent(
    mozilla::dom::EventTarget* aOwner);

  





  virtual nsresult OnSuccess();

  




  virtual void OnError();

  



  virtual nsresult GetSuccessResult(JSContext* aCx,
                                    jsval* aVal) MOZ_OVERRIDE;

  




  virtual void ReleaseMainThreadObjects() MOZ_OVERRIDE;

  


  static nsresult ConvertToArrayAndCleanup(
                                JSContext* aCx,
                                nsTArray<StructuredCloneReadInfo>& aReadInfos,
                                jsval* aResult);

  


  static void SetCurrentTransaction(IDBTransaction* aTransaction);

  




  virtual ChildProcessSendResult
  SendResponseToChildProcess(nsresult aResultCode) = 0;

protected:
  nsRefPtr<IDBDatabase> mDatabase;
  nsRefPtr<IDBTransaction> mTransaction;

private:
  nsCOMPtr<mozIStorageProgressHandler> mOldProgressHandler;
  nsresult mResultCode;
  bool mDispatched;
};

NS_STACK_CLASS
class StackBasedEventTarget : public nsIEventTarget
{
public:
  NS_DECL_ISUPPORTS_INHERITED
};

class MainThreadEventTarget : public StackBasedEventTarget
{
public:
  NS_DECL_NSIEVENTTARGET
};

class NoDispatchEventTarget : public StackBasedEventTarget
{
public:
  NS_DECL_NSIEVENTTARGET
};

END_INDEXEDDB_NAMESPACE

#endif
