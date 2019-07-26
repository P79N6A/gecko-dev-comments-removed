




#ifndef mozilla_dom_workers_eventtarget_h__
#define mozilla_dom_workers_eventtarget_h__

#include "mozilla/dom/workers/bindings/DOMBindingBase.h"


#include "mozilla/dom/workers/bindings/EventListenerManager.h"

#include "mozilla/dom/Nullable.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {
class EventListener;
class EventHandlerNonNull;
} 
} 

BEGIN_WORKERS_NAMESPACE

class EventTarget : public DOMBindingBase
{
  EventListenerManager mListenerManager;

protected:
  EventTarget(JSContext* aCx)
  : DOMBindingBase(aCx)
  { }

  virtual ~EventTarget()
  { }

public:
  virtual void
  _trace(JSTracer* aTrc) MOZ_OVERRIDE;

  virtual void
  _finalize(JSFreeOp* aFop) MOZ_OVERRIDE;

  void
  AddEventListener(const nsAString& aType, EventListener* aListener,
                   bool aCapture, Nullable<bool> aWantsUntrusted,
                   ErrorResult& aRv);

  void
  RemoveEventListener(const nsAString& aType, EventListener* aListener,
                      bool aCapture, ErrorResult& aRv);

  bool
  DispatchEvent(JS::Handle<JSObject*> aEvent, ErrorResult& aRv) const
  {
    return mListenerManager.DispatchEvent(GetJSContext(), *this, aEvent, aRv);
  }

  already_AddRefed<EventHandlerNonNull>
  GetEventListener(const nsAString& aType, ErrorResult& aRv) const;

  void
  SetEventListener(const nsAString& aType, EventHandlerNonNull* aListener,
                   ErrorResult& aRv);

  bool
  HasListeners() const
  {
    return mListenerManager.HasListeners();
  }

  void SetEventHandler(const nsAString& aType, EventHandlerNonNull* aHandler,
                       ErrorResult& rv)
  {
    rv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  }

  EventHandlerNonNull*
  GetEventHandler(const nsAString& aType)
  {
    return nullptr;
  }

  JSObject* GetOwnerGlobal() const
  {
    
    return nullptr;
  }
};

END_WORKERS_NAMESPACE

#endif 
