





#include "IDBEvents.h"

#include "nsJSON.h"
#include "nsThreadUtils.h"

#include "IDBRequest.h"
#include "IDBTransaction.h"

USING_INDEXEDDB_NAMESPACE
using namespace mozilla::dom;

namespace {

class EventFiringRunnable : public nsRunnable
{
public:
  EventFiringRunnable(EventTarget* aTarget,
                      nsIDOMEvent* aEvent)
  : mTarget(aTarget), mEvent(aEvent)
  { }

  NS_IMETHOD Run() {
    bool dummy;
    return mTarget->DispatchEvent(mEvent, &dummy);
  }

private:
  nsCOMPtr<EventTarget> mTarget;
  nsCOMPtr<nsIDOMEvent> mEvent;
};

} 

already_AddRefed<nsIDOMEvent>
mozilla::dom::indexedDB::CreateGenericEvent(mozilla::dom::EventTarget* aOwner,
                                            const nsAString& aType,
                                            Bubbles aBubbles,
                                            Cancelable aCancelable)
{
  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMEvent(getter_AddRefs(event), aOwner, nullptr, nullptr);
  nsresult rv = event->InitEvent(aType,
                                 aBubbles == eDoesBubble ? true : false,
                                 aCancelable == eCancelable ? true : false);
  NS_ENSURE_SUCCESS(rv, nullptr);

  event->SetTrusted(true);

  return event.forget();
}


already_AddRefed<IDBVersionChangeEvent>
IDBVersionChangeEvent::CreateInternal(mozilla::dom::EventTarget* aOwner,
                                      const nsAString& aType,
                                      uint64_t aOldVersion,
                                      uint64_t aNewVersion)
{
  nsRefPtr<IDBVersionChangeEvent> event(new IDBVersionChangeEvent(aOwner));

  nsresult rv = event->InitEvent(aType, false, false);
  NS_ENSURE_SUCCESS(rv, nullptr);

  event->SetTrusted(true);

  event->mOldVersion = aOldVersion;
  event->mNewVersion = aNewVersion;

  return event.forget();
}


already_AddRefed<nsIRunnable>
IDBVersionChangeEvent::CreateRunnableInternal(mozilla::dom::EventTarget* aTarget,
                                              const nsAString& aType,
                                              uint64_t aOldVersion,
                                              uint64_t aNewVersion)
{
  nsRefPtr<Event> event =
    CreateInternal(aTarget, aType, aOldVersion, aNewVersion);
  NS_ENSURE_TRUE(event, nullptr);

  nsCOMPtr<nsIRunnable> runnable(new EventFiringRunnable(aTarget, event));
  return runnable.forget();
}

NS_IMPL_ADDREF_INHERITED(IDBVersionChangeEvent, Event)
NS_IMPL_RELEASE_INHERITED(IDBVersionChangeEvent, Event)

NS_INTERFACE_MAP_BEGIN(IDBVersionChangeEvent)
  NS_INTERFACE_MAP_ENTRY(IDBVersionChangeEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)
