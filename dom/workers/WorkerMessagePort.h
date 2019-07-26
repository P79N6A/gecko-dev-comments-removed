




#ifndef mozilla_dom_workers_workermessageport_h__
#define mozilla_dom_workers_workermessageport_h__

#include "js/StructuredClone.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/workers/bindings/EventTarget.h"

BEGIN_WORKERS_NAMESPACE

class WorkerPrivate;

class WorkerMessagePort : public EventTarget
{
  friend class WorkerPrivate;

  typedef mozilla::ErrorResult ErrorResult;

  struct MessageInfo
  {
    JSAutoStructuredCloneBuffer mBuffer;
    nsTArray<nsCOMPtr<nsISupports>> mClonedObjects;
  };

  nsTArray<MessageInfo> mQueuedMessages;
  uint64_t mSerial;
  bool mStarted;
  bool mClosed;

public:
  virtual void
  _trace(JSTracer* aTrc) MOZ_OVERRIDE;

  virtual void
  _finalize(JSFreeOp* aFop) MOZ_OVERRIDE;

  void
  PostMessage(JSContext* aCx, JS::HandleValue aMessage,
              const Optional<Sequence<JS::Value>>& aTransferable,
              ErrorResult& aRv);

  void
  Start();

  void
  Close()
  {
    if (!IsClosed()) {
      CloseInternal();
    }
  }

  already_AddRefed<EventHandlerNonNull>
  GetOnmessage(ErrorResult& aRv)
  {
    return GetEventListener(NS_LITERAL_STRING("message"), aRv);
  }

  void
  SetOnmessage(EventHandlerNonNull* aListener, ErrorResult& aRv)
  {
    SetEventListener(NS_LITERAL_STRING("message"), aListener, aRv);
    if (!aRv.Failed()) {
      Start();
    }
  }

  uint64_t
  Serial() const
  {
    return mSerial;
  }

  bool
  MaybeDispatchEvent(JSContext* aCx, JSAutoStructuredCloneBuffer& aBuffer,
                     nsTArray<nsCOMPtr<nsISupports>>& aClonedObjects);

  bool
  IsClosed() const
  {
    return mClosed;
  }

private:
  
  WorkerMessagePort(JSContext* aCx, uint64_t aSerial)
  : EventTarget(aCx), mSerial(aSerial), mStarted(false), mClosed(false)
  { }

  virtual ~WorkerMessagePort()
  { }

  void
  CloseInternal();
};

END_WORKERS_NAMESPACE

#endif 
