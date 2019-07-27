




#ifndef mozilla_dom_workers_serviceworkerevents_h__
#define mozilla_dom_workers_serviceworkerevents_h__

#include "mozilla/dom/Event.h"
#include "mozilla/dom/InstallPhaseEventBinding.h"
#include "mozilla/dom/InstallEventBinding.h"

namespace mozilla {
namespace dom {
  class Promise;
} 
} 

BEGIN_WORKERS_NAMESPACE

class ServiceWorker;

bool
ServiceWorkerEventsVisible(JSContext* aCx, JSObject* aObj);

class InstallPhaseEvent : public Event
{
  nsRefPtr<Promise> mPromise;

protected:
  explicit InstallPhaseEvent(mozilla::dom::EventTarget* aOwner);
  ~InstallPhaseEvent() {}

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(InstallPhaseEvent, Event)
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return mozilla::dom::InstallPhaseEventBinding_workers::Wrap(aCx, this);
  }

  static already_AddRefed<InstallPhaseEvent>
  Constructor(mozilla::dom::EventTarget* aOwner,
              const nsAString& aType,
              const EventInit& aOptions)
  {
    nsRefPtr<InstallPhaseEvent> e = new InstallPhaseEvent(aOwner);
    bool trusted = e->Init(aOwner);
    e->InitEvent(aType, aOptions.mBubbles, aOptions.mCancelable);
    e->SetTrusted(trusted);
    return e.forget();
  }

  static already_AddRefed<InstallPhaseEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const EventInit& aOptions,
              ErrorResult& aRv)
  {
    nsCOMPtr<EventTarget> target = do_QueryInterface(aGlobal.GetAsSupports());
    return Constructor(target, aType, aOptions);
  }

  void
  WaitUntil(Promise& aPromise);

  already_AddRefed<Promise>
  GetPromise() const
  {
    nsRefPtr<Promise> p = mPromise;
    return p.forget();
  }
};

class InstallEvent MOZ_FINAL : public InstallPhaseEvent
{
  
  nsRefPtr<ServiceWorker> mActiveWorker;

protected:
  explicit InstallEvent(mozilla::dom::EventTarget* aOwner);
  ~InstallEvent() {}

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(InstallEvent, InstallPhaseEvent)
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return mozilla::dom::InstallEventBinding_workers::Wrap(aCx, this);
  }

  static already_AddRefed<InstallEvent>
  Constructor(mozilla::dom::EventTarget* aOwner,
              const nsAString& aType,
              const InstallEventInit& aOptions)
  {
    nsRefPtr<InstallEvent> e = new InstallEvent(aOwner);
    bool trusted = e->Init(aOwner);
    e->InitEvent(aType, aOptions.mBubbles, aOptions.mCancelable);
    e->SetTrusted(trusted);
    e->mActiveWorker = aOptions.mActiveWorker;
    return e.forget();
  }

  static already_AddRefed<InstallEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const InstallEventInit& aOptions,
              ErrorResult& aRv)
  {
    nsCOMPtr<EventTarget> owner = do_QueryInterface(aGlobal.GetAsSupports());
    return Constructor(owner, aType, aOptions);
  }

  already_AddRefed<ServiceWorker>
  GetActiveWorker() const
  {
    nsRefPtr<ServiceWorker> sw = mActiveWorker;
    return sw.forget();
  }

  void
  Replace()
  {
    
    NS_WARNING("Not Implemented");
  };
};

END_WORKERS_NAMESPACE
#endif 
