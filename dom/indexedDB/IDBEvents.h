






































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

#include "nsDOMEvent.h"

#define SUCCESS_EVT_STR "success"
#define ERROR_EVT_STR "error"

BEGIN_INDEXEDDB_NAMESPACE

class IDBRequest;

class IDBEvent : public nsDOMEvent,
                 public nsIIDBEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBEVENT
  NS_FORWARD_TO_NSDOMEVENT

protected:
  IDBEvent() : nsDOMEvent(nsnull, nsnull) { }

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
                        public nsIIDBSuccessEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBSUCCESSEVENT
  NS_FORWARD_NSIDOMEVENT(IDBEvent::)
  NS_FORWARD_NSIIDBEVENT(IDBEvent::)

  static already_AddRefed<nsIDOMEvent>
  Create(IDBRequest* aRequest,
         nsIVariant* aResult);

  static already_AddRefed<nsIRunnable>
  CreateRunnable(IDBRequest* aRequest,
                 nsIVariant* aResult);

protected:
  IDBSuccessEvent() { }

  nsCOMPtr<nsIVariant> mResult;
};

class IDBTransactionEvent : public IDBSuccessEvent,
                            public nsIIDBTransactionEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBTRANSACTIONEVENT
  NS_FORWARD_NSIDOMEVENT(IDBSuccessEvent::)
  NS_FORWARD_NSIIDBEVENT(IDBSuccessEvent::)
  NS_FORWARD_NSIIDBSUCCESSEVENT(IDBSuccessEvent::)

  static already_AddRefed<nsIDOMEvent>
  Create(IDBRequest* aRequest,
         nsIVariant* aResult,
         nsIIDBTransactionRequest* aTransaction);

  static already_AddRefed<nsIRunnable>
  CreateRunnable(IDBRequest* aRequest,
                 nsIVariant* aResult,
                 nsIIDBTransactionRequest* aTransaction);

protected:
  IDBTransactionEvent() { }

  nsCOMPtr<nsIIDBTransactionRequest> mTransaction;
};

END_INDEXEDDB_NAMESPACE

#endif 
