






































#include "IDBEvents.h"

#include "nsIIDBDatabaseException.h"
#include "nsIPrivateDOMEvent.h"

#include "jscntxt.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsDOMException.h"
#include "nsJSON.h"
#include "nsThreadUtils.h"

#include "IDBRequest.h"
#include "IDBTransaction.h"

USING_INDEXEDDB_NAMESPACE

namespace {

class EventFiringRunnable : public nsRunnable
{
public:
  EventFiringRunnable(nsIDOMEventTarget* aTarget,
                      nsIDOMEvent* aEvent)
  : mTarget(aTarget), mEvent(aEvent)
  { }

  NS_IMETHOD Run() {
    bool dummy;
    return mTarget->DispatchEvent(mEvent, &dummy);
  }

private:
  nsCOMPtr<nsIDOMEventTarget> mTarget;
  nsCOMPtr<nsIDOMEvent> mEvent;
};

} 

already_AddRefed<nsDOMEvent>
mozilla::dom::indexedDB::CreateGenericEvent(const nsAString& aType,
                                            bool aBubblesAndCancelable)
{
  nsRefPtr<nsDOMEvent> event(new nsDOMEvent(nsnull, nsnull));
  nsresult rv = event->InitEvent(aType, aBubblesAndCancelable,
                                 aBubblesAndCancelable);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = event->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, nsnull);

  return event.forget();
}

already_AddRefed<nsIRunnable>
mozilla::dom::indexedDB::CreateGenericEventRunnable(const nsAString& aType,
                                                    nsIDOMEventTarget* aTarget)
{
  nsCOMPtr<nsIDOMEvent> event(CreateGenericEvent(aType));
  NS_ENSURE_TRUE(event, nsnull);

  nsCOMPtr<nsIRunnable> runnable(new EventFiringRunnable(aTarget, event));
  return runnable.forget();
}


already_AddRefed<nsIDOMEvent>
IDBVersionChangeEvent::CreateInternal(const nsAString& aType,
                                      const nsAString& aVersion)
{
  nsRefPtr<IDBVersionChangeEvent> event(new IDBVersionChangeEvent());

  nsresult rv = event->InitEvent(aType, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = event->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, nsnull);

  event->mVersion = aVersion;

  nsDOMEvent* result;
  event.forget(&result);
  return result;
}


already_AddRefed<nsIRunnable>
IDBVersionChangeEvent::CreateRunnableInternal(const nsAString& aType,
                                              const nsAString& aVersion,
                                              nsIDOMEventTarget* aTarget)
{
  nsCOMPtr<nsIDOMEvent> event = CreateInternal(aType, aVersion);
  NS_ENSURE_TRUE(event, nsnull);

  nsCOMPtr<nsIRunnable> runnable(new EventFiringRunnable(aTarget, event));
  return runnable.forget();
}

NS_IMPL_ADDREF_INHERITED(IDBVersionChangeEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(IDBVersionChangeEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(IDBVersionChangeEvent)
  NS_INTERFACE_MAP_ENTRY(nsIIDBVersionChangeEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBVersionChangeEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

DOMCI_DATA(IDBVersionChangeEvent, IDBVersionChangeEvent)

NS_IMETHODIMP
IDBVersionChangeEvent::GetVersion(nsAString& aVersion)
{
  aVersion.Assign(mVersion);
  return NS_OK;
}
