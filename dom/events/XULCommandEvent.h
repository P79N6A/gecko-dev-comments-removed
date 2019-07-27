







#ifndef mozilla_dom_XULCommandEvent_h_
#define mozilla_dom_XULCommandEvent_h_

#include "mozilla/dom/UIEvent.h"
#include "mozilla/dom/XULCommandEventBinding.h"
#include "nsIDOMXULCommandEvent.h"

namespace mozilla {
namespace dom {

class XULCommandEvent : public UIEvent,
                        public nsIDOMXULCommandEvent
{
public:
  XULCommandEvent(EventTarget* aOwner,
                  nsPresContext* aPresContext,
                  WidgetInputEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULCommandEvent, UIEvent)
  NS_DECL_NSIDOMXULCOMMANDEVENT

  
  NS_FORWARD_TO_UIEVENT

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return XULCommandEventBinding::Wrap(aCx, this);
  }

  bool AltKey();
  bool CtrlKey();
  bool ShiftKey();
  bool MetaKey();

  already_AddRefed<Event> GetSourceEvent()
  {
    nsRefPtr<Event> e =
      mSourceEvent ? mSourceEvent->InternalDOMEvent() : nullptr;
    return e.forget();
  }

  void InitCommandEvent(const nsAString& aType,
                        bool aCanBubble, bool aCancelable,
                        nsIDOMWindow* aView,
                        int32_t aDetail,
                        bool aCtrlKey, bool aAltKey,
                        bool aShiftKey, bool aMetaKey,
                        Event* aSourceEvent,
                        ErrorResult& aRv)
  {
    aRv = InitCommandEvent(aType, aCanBubble, aCancelable, aView, aDetail,
                           aCtrlKey, aAltKey, aShiftKey, aMetaKey,
                           aSourceEvent);
  }

protected:
  ~XULCommandEvent() {}

  nsCOMPtr<nsIDOMEvent> mSourceEvent;
};

} 
} 

#endif 
