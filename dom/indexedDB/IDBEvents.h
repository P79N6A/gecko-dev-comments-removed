






































#ifndef mozilla_dom_indexeddb_idbevents_h__
#define mozilla_dom_indexeddb_idbevents_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBVersionChangeEvent.h"
#include "nsIRunnable.h"

#include "nsDOMEvent.h"

#include "mozilla/dom/indexedDB/IDBObjectStore.h"

#define SUCCESS_EVT_STR "success"
#define ERROR_EVT_STR "error"
#define COMPLETE_EVT_STR "complete"
#define ABORT_EVT_STR "abort"
#define TIMEOUT_EVT_STR "timeout"
#define VERSIONCHANGE_EVT_STR "versionchange"
#define BLOCKED_EVT_STR "blocked"

BEGIN_INDEXEDDB_NAMESPACE

already_AddRefed<nsDOMEvent>
CreateGenericEvent(const nsAString& aType,
                   bool aBubblesAndCancelable = false);

already_AddRefed<nsIRunnable>
CreateGenericEventRunnable(const nsAString& aType,
                           nsIDOMEventTarget* aTarget);

class IDBVersionChangeEvent : public nsDOMEvent,
                              public nsIIDBVersionChangeEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_NSIIDBVERSIONCHANGEEVENT

  inline static already_AddRefed<nsIDOMEvent>
  Create(const nsAString& aVersion)
  {
    return CreateInternal(NS_LITERAL_STRING(VERSIONCHANGE_EVT_STR), aVersion);
  }

  inline static already_AddRefed<nsIDOMEvent>
  CreateBlocked(const nsAString& aVersion)
  {
    return CreateInternal(NS_LITERAL_STRING(BLOCKED_EVT_STR), aVersion);
  }

  inline static already_AddRefed<nsIRunnable>
  CreateRunnable(const nsAString& aVersion,
                 nsIDOMEventTarget* aTarget)
  {
    return CreateRunnableInternal(NS_LITERAL_STRING(VERSIONCHANGE_EVT_STR),
                                  aVersion, aTarget);
  }

  static already_AddRefed<nsIRunnable>
  CreateBlockedRunnable(const nsAString& aVersion,
                        nsIDOMEventTarget* aTarget)
  {
    return CreateRunnableInternal(NS_LITERAL_STRING(BLOCKED_EVT_STR), aVersion,
                                  aTarget);
  }

protected:
  IDBVersionChangeEvent() : nsDOMEvent(nsnull, nsnull) { }
  virtual ~IDBVersionChangeEvent() { }

  static already_AddRefed<nsIDOMEvent>
  CreateInternal(const nsAString& aType,
                 const nsAString& aVersion);

  static already_AddRefed<nsIRunnable>
  CreateRunnableInternal(const nsAString& aType,
                         const nsAString& aVersion,
                         nsIDOMEventTarget* aTarget);

  nsString mVersion;
};

END_INDEXEDDB_NAMESPACE

#endif 
