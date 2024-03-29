




#ifndef mozilla_dom_workers_notificationevent_h__
#define mozilla_dom_workers_notificationevent_h__

#include "mozilla/dom/Event.h"
#include "mozilla/dom/NotificationEventBinding.h"
#include "mozilla/dom/ServiceWorkerEvents.h"
#include "mozilla/dom/workers/Workers.h"

BEGIN_WORKERS_NAMESPACE

class ServiceWorker;
class ServiceWorkerClient;

class NotificationEvent final : public ExtendableEvent
{
protected:
  explicit NotificationEvent(EventTarget* aOwner);
  ~NotificationEvent()
  {}

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(NotificationEvent, ExtendableEvent)
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return NotificationEventBinding::Wrap(aCx, this, aGivenProto);
  }

  static already_AddRefed<NotificationEvent>
  Constructor(mozilla::dom::EventTarget* aOwner,
              const nsAString& aType,
              const NotificationEventInit& aOptions,
              ErrorResult& aRv)
  {
    nsRefPtr<NotificationEvent> e = new NotificationEvent(aOwner);
    bool trusted = e->Init(aOwner);
    e->InitEvent(aType, aOptions.mBubbles, aOptions.mCancelable);
    e->SetTrusted(trusted);
    e->mNotification = aOptions.mNotification;
    e->SetWantsPopupControlCheck(e->IsTrusted());
    return e.forget();
  }

  static already_AddRefed<NotificationEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const NotificationEventInit& aOptions,
              ErrorResult& aRv)
  {
    nsCOMPtr<EventTarget> owner = do_QueryInterface(aGlobal.GetAsSupports());
    return Constructor(owner, aType, aOptions, aRv);
  }

  already_AddRefed<Notification>
  Notification_()
  {
    nsRefPtr<Notification> n = mNotification;
    return n.forget();
  }

private:
  nsRefPtr<Notification> mNotification;
};

END_WORKERS_NAMESPACE
#endif 

