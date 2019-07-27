




#ifndef mozilla_dom_workers_serviceworkerevents_h__
#define mozilla_dom_workers_serviceworkerevents_h__

#include "mozilla/dom/Event.h"
#include "mozilla/dom/ExtendableEventBinding.h"
#include "mozilla/dom/InstallEventBinding.h"

namespace mozilla {
namespace dom {
  class Promise;
} 
} 

BEGIN_WORKERS_NAMESPACE

class ServiceWorker;

class ExtendableEvent : public Event
{
  nsRefPtr<Promise> mPromise;

protected:
  explicit ExtendableEvent(mozilla::dom::EventTarget* aOwner);
  ~ExtendableEvent() {}

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ExtendableEvent, Event)
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObjectInternal(JSContext* aCx) MOZ_OVERRIDE
  {
    return mozilla::dom::ExtendableEventBinding::Wrap(aCx, this);
  }

  static already_AddRefed<ExtendableEvent>
  Constructor(mozilla::dom::EventTarget* aOwner,
              const nsAString& aType,
              const EventInit& aOptions)
  {
    nsRefPtr<ExtendableEvent> e = new ExtendableEvent(aOwner);
    bool trusted = e->Init(aOwner);
    e->InitEvent(aType, aOptions.mBubbles, aOptions.mCancelable);
    e->SetTrusted(trusted);
    return e.forget();
  }

  static already_AddRefed<ExtendableEvent>
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

  virtual ExtendableEvent* AsExtendableEvent() MOZ_OVERRIDE
  {
    return this;
  }
};

class InstallEvent MOZ_FINAL : public ExtendableEvent
{
  
  nsRefPtr<ServiceWorker> mActiveWorker;
  bool mActivateImmediately;

protected:
  explicit InstallEvent(mozilla::dom::EventTarget* aOwner);
  ~InstallEvent() {}

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(InstallEvent, ExtendableEvent)
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObjectInternal(JSContext* aCx) MOZ_OVERRIDE
  {
    return mozilla::dom::InstallEventBinding::Wrap(aCx, this);
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
    mActivateImmediately = true;
  };

  bool
  ActivateImmediately() const
  {
    return mActivateImmediately;
  }

  InstallEvent* AsInstallEvent() MOZ_OVERRIDE
  {
    return this;
  }
};

END_WORKERS_NAMESPACE
#endif 
