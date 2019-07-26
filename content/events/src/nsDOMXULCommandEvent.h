







#ifndef nsDOMXULCommandEvent_h_
#define nsDOMXULCommandEvent_h_

#include "nsDOMUIEvent.h"
#include "nsIDOMXULCommandEvent.h"
#include "mozilla/dom/XULCommandEventBinding.h"

class nsDOMXULCommandEvent : public nsDOMUIEvent,
                             public nsIDOMXULCommandEvent
{
public:
  nsDOMXULCommandEvent(mozilla::dom::EventTarget* aOwner,
                       nsPresContext* aPresContext,
                       mozilla::WidgetInputEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMXULCommandEvent, nsDOMUIEvent)
  NS_DECL_NSIDOMXULCOMMANDEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::XULCommandEventBinding::Wrap(aCx, aScope, this);
  }

  bool AltKey()
  {
    return Event()->IsAlt();
  }

  bool CtrlKey()
  {
    return Event()->IsControl();
  }

  bool ShiftKey()
  {
    return Event()->IsShift();
  }

  bool MetaKey()
  {
    return Event()->IsMeta();
  }

  already_AddRefed<nsDOMEvent> GetSourceEvent()
  {
    nsRefPtr<nsDOMEvent> e =
      mSourceEvent ? mSourceEvent->InternalDOMEvent() : nullptr;
    return e.forget();
  }

  void InitCommandEvent(const nsAString& aType,
                        bool aCanBubble, bool aCancelable,
                        nsIDOMWindow* aView,
                        int32_t aDetail,
                        bool aCtrlKey, bool aAltKey,
                        bool aShiftKey, bool aMetaKey,
                        nsDOMEvent* aSourceEvent,
                        mozilla::ErrorResult& aRv)
  {
    aRv = InitCommandEvent(aType, aCanBubble, aCancelable, aView, aDetail,
                           aCtrlKey, aAltKey, aShiftKey, aMetaKey,
                           aSourceEvent);
  }

protected:
  
  mozilla::WidgetInputEvent* Event() {
    return static_cast<mozilla::WidgetInputEvent*>(mEvent);
  }

  nsCOMPtr<nsIDOMEvent> mSourceEvent;
};

#endif  
