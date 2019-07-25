






































#ifndef mozilla_dom_indexeddb_idbevents_h__
#define mozilla_dom_indexeddb_idbevents_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBEvent.h"
#include "nsIIDBErrorEvent.h"
#include "nsIIDBSuccessEvent.h"
#include "nsIIDBTransactionEvent.h"
#include "nsIIDBTransactionRequest.h"
#include "nsIRunnable.h"
#include "nsIVariant.h"

#include "jsapi.h"
#include "nsDOMEvent.h"

#include "mozilla/dom/indexedDB/IDBObjectStoreRequest.h"

#define SUCCESS_EVT_STR "success"
#define ERROR_EVT_STR "error"
#define COMPLETE_EVT_STR "complete"
#define ABORT_EVT_STR "abort"
#define TIMEOUT_EVT_STR "timeout"

BEGIN_INDEXEDDB_NAMESPACE

class IDBRequest;
class IDBTransactionRequest;

class IDBEvent : public nsDOMEvent,
                 public nsIIDBEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBEVENT
  NS_FORWARD_TO_NSDOMEVENT

  static already_AddRefed<nsIDOMEvent>
  CreateGenericEvent(const nsAString& aType);

  static already_AddRefed<nsIRunnable>
  CreateGenericEventRunnable(const nsAString& aType,
                             nsIDOMEventTarget* aTarget);

protected:
  IDBEvent() : nsDOMEvent(nsnull, nsnull) { }
  virtual ~IDBEvent() { }

  nsCOMPtr<nsISupports> mSource;
};

class IDBErrorEvent : public IDBEvent,
                      public nsIIDBErrorEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBERROREVENT
  NS_FORWARD_NSIDOMEVENT(IDBEvent::)
  NS_FORWARD_NSIIDBEVENT(IDBEvent::)

  static already_AddRefed<nsIDOMEvent>
  Create(IDBRequest* aRequest,
         PRUint16 aCode);

  static already_AddRefed<nsIRunnable>
  CreateRunnable(IDBRequest* aRequest,
                 PRUint16 aCode);

protected:
  IDBErrorEvent() { }

  PRUint16 mCode;
  nsString mMessage;
};

class IDBSuccessEvent : public IDBEvent,
                        public nsIIDBTransactionEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBSUCCESSEVENT
  NS_DECL_NSIIDBTRANSACTIONEVENT
  NS_FORWARD_NSIDOMEVENT(IDBEvent::)
  NS_FORWARD_NSIIDBEVENT(IDBEvent::)

  static already_AddRefed<nsIDOMEvent>
  Create(IDBRequest* aRequest,
         nsIVariant* aResult,
         nsIIDBTransactionRequest* aTransaction);

  static already_AddRefed<nsIRunnable>
  CreateRunnable(IDBRequest* aRequest,
                 nsIVariant* aResult,
                 nsIIDBTransactionRequest* aTransaction);

protected:
  IDBSuccessEvent() { }

  nsCOMPtr<nsIVariant> mResult;
  nsCOMPtr<nsIIDBTransactionRequest> mTransaction;
};

class GetSuccessEvent : public IDBSuccessEvent
{
public:
  GetSuccessEvent(const nsAString& aValue)
  : mValue(aValue),
    mCachedValue(JSVAL_VOID),
    mJSContext(nsnull)
  { }

  ~GetSuccessEvent()
  {
    if (mJSContext) {
      JSAutoRequest ar(mJSContext);
      JS_RemoveValueRoot(mJSContext, &mCachedValue);
    }
  }

  NS_IMETHOD GetResult(nsIVariant** aResult);

  nsresult Init(IDBRequest* aRequest,
                IDBTransactionRequest* aTransaction);

private:
  nsString mValue;

protected:
  jsval mCachedValue;
  JSContext* mJSContext;
};

class GetAllSuccessEvent : public GetSuccessEvent
{
public:
  GetAllSuccessEvent(nsTArray<nsString>& aValues)
  : GetSuccessEvent(EmptyString())
  {
    if (!mValues.SwapElements(aValues)) {
      NS_ERROR("Failed to swap elements!");
    }
  }

  NS_IMETHOD GetResult(nsIVariant** aResult);

private:
  nsTArray<nsString> mValues;
};

class GetAllKeySuccessEvent : public GetSuccessEvent
{
public:
  GetAllKeySuccessEvent(nsTArray<Key>& aKeys)
  : GetSuccessEvent(EmptyString())
  {
    if (!mKeys.SwapElements(aKeys)) {
      NS_ERROR("Failed to swap elements!");
    }
  }

  NS_IMETHOD GetResult(nsIVariant** aResult);

private:
  nsTArray<Key> mKeys;
};

END_INDEXEDDB_NAMESPACE

#endif 
