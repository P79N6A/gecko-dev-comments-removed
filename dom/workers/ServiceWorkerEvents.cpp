





#include "ServiceWorkerEvents.h"

#include "nsContentUtils.h"

#include "mozilla/dom/Promise.h"
#include "mozilla/dom/WorkerScope.h"
#include "mozilla/dom/workers/bindings/ServiceWorker.h"
#include "mozilla/dom/ServiceWorkerGlobalScopeBinding.h"

using namespace mozilla::dom;

BEGIN_WORKERS_NAMESPACE

bool
ServiceWorkerEventsVisible(JSContext* aCx, JSObject* aObj)
{
  ServiceWorkerGlobalScope* scope = nullptr;
  nsresult rv = UnwrapObject<prototypes::id::ServiceWorkerGlobalScope_workers,
                             mozilla::dom::ServiceWorkerGlobalScopeBinding_workers::NativeType>(aObj, scope);
  return NS_SUCCEEDED(rv) && scope;
}

InstallPhaseEvent::InstallPhaseEvent(EventTarget* aOwner)
  : Event(aOwner, nullptr, nullptr)
{
}

void
InstallPhaseEvent::WaitUntil(Promise& aPromise)
{
  MOZ_ASSERT(!NS_IsMainThread());

  
  if (EventPhase() == AT_TARGET && !mPromise) {
    mPromise = &aPromise;
  }
}

NS_IMPL_ADDREF_INHERITED(InstallPhaseEvent, Event)
NS_IMPL_RELEASE_INHERITED(InstallPhaseEvent, Event)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(InstallPhaseEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)

NS_IMPL_CYCLE_COLLECTION_INHERITED(InstallPhaseEvent, Event, mPromise)

InstallEvent::InstallEvent(EventTarget* aOwner)
  : InstallPhaseEvent(aOwner)
{
}

NS_IMPL_ADDREF_INHERITED(InstallEvent, InstallPhaseEvent)
NS_IMPL_RELEASE_INHERITED(InstallEvent, InstallPhaseEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(InstallEvent)
NS_INTERFACE_MAP_END_INHERITING(InstallPhaseEvent)

NS_IMPL_CYCLE_COLLECTION_INHERITED(InstallEvent, InstallPhaseEvent, mActiveWorker)

END_WORKERS_NAMESPACE
