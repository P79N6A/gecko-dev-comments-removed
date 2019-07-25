






































#ifndef mozilla_dom_indexeddb_idbevents_h__
#define mozilla_dom_indexeddb_idbevents_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBDatabaseError.h"
#include "nsIIDBErrorEvent.h"
#include "nsIIDBSuccessEvent.h"
#include "nsIRunnable.h"
#include "nsIVariant.h"

#include "nsDOMEvent.h"

#define SUCCESS_EVT_STR "success"
#define ERROR_EVT_STR "error"

class nsIDOMEventTarget;

BEGIN_INDEXEDDB_NAMESPACE

class IDBErrorEvent : public nsDOMEvent,
                      public nsIIDBErrorEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBERROREVENT
  NS_FORWARD_TO_NSDOMEVENT

  static already_AddRefed<nsIDOMEvent>
  Create(PRUint16 aCode);

  static already_AddRefed<nsIRunnable>
  CreateRunnable(nsIDOMEventTarget* aTarget,
                 PRUint16 aCode);

protected:
  IDBErrorEvent()
  : nsDOMEvent(nsnull, nsnull)
  { }

  nsresult
  Init();

  nsCOMPtr<nsIIDBDatabaseError> mError;
};

class IDBSuccessEvent : public nsDOMEvent,
                        public nsIIDBSuccessEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBSUCCESSEVENT
  NS_FORWARD_TO_NSDOMEVENT

  static already_AddRefed<nsIDOMEvent>
  Create(nsIVariant* aResult);

  static already_AddRefed<nsIRunnable>
  CreateRunnable(nsIDOMEventTarget* aTarget,
                 nsIVariant* aResult);

protected:
  IDBSuccessEvent()
  : nsDOMEvent(nsnull, nsnull)
  { }

  nsresult
  Init();

  nsCOMPtr<nsIVariant> mResult;
};

END_INDEXEDDB_NAMESPACE

#endif 
