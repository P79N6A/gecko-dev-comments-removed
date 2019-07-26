




#ifndef mozilla_dom_workers_messageport_h_
#define mozilla_dom_workers_messageport_h_

#include "Workers.h"

#include "mozilla/dom/BindingDeclarations.h"
#include "nsDOMEventTargetHelper.h"

class nsIDOMEvent;
class nsPIDOMWindow;

BEGIN_WORKERS_NAMESPACE

class SharedWorker;

class MessagePort MOZ_FINAL : public nsDOMEventTargetHelper
{
  friend class SharedWorker;

  typedef mozilla::ErrorResult ErrorResult;

  nsRefPtr<SharedWorker> mSharedWorker;
  nsTArray<nsCOMPtr<nsIDOMEvent>> mQueuedEvents;
  uint64_t mSerial;
  bool mStarted;

public:
  static bool
  PrefEnabled();

  void
  PostMessage(JSContext* aCx, JS::HandleValue aMessage,
              const Optional<Sequence<JS::Value>>& aTransferable,
              ErrorResult& aRv);

  void
  Start();

  void
  Close()
  {
    AssertIsOnMainThread();

    if (!IsClosed()) {
      CloseInternal();
    }
  }

  uint64_t
  Serial() const
  {
    return mSerial;
  }

  void
  QueueEvent(nsIDOMEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MessagePort, nsDOMEventTargetHelper)

  EventHandlerNonNull*
  GetOnmessage()
  {
    AssertIsOnMainThread();

    return GetEventHandler(nsGkAtoms::onmessage, EmptyString());
  }

  void
  SetOnmessage(EventHandlerNonNull* aCallback)
  {
    AssertIsOnMainThread();

    SetEventHandler(nsGkAtoms::onmessage, EmptyString(), aCallback);

    Start();
  }

  bool
  IsClosed() const
  {
    AssertIsOnMainThread();

    return !mSharedWorker;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::HandleObject aScope) MOZ_OVERRIDE;

  virtual nsresult
  PreHandleEvent(nsEventChainPreVisitor& aVisitor) MOZ_OVERRIDE;

private:
  
  MessagePort(nsPIDOMWindow* aWindow, SharedWorker* aSharedWorker,
              uint64_t aSerial);

  
  ~MessagePort();

  void
  CloseInternal();
};

END_WORKERS_NAMESPACE

#endif 
