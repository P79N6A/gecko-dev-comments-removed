





#ifndef mozilla_dom_CompositionEvent_h_
#define mozilla_dom_CompositionEvent_h_

#include "mozilla/dom/CompositionEventBinding.h"
#include "mozilla/dom/UIEvent.h"
#include "mozilla/EventForwards.h"
#include "nsIDOMCompositionEvent.h"

namespace mozilla {
namespace dom {

class CompositionEvent : public UIEvent,
                         public nsIDOMCompositionEvent
{
public:
  CompositionEvent(EventTarget* aOwner,
                   nsPresContext* aPresContext,
                   WidgetCompositionEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_UIEVENT
  NS_DECL_NSIDOMCOMPOSITIONEVENT

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return CompositionEventBinding::Wrap(aCx, this);
  }

  void InitCompositionEvent(const nsAString& aType,
                            bool aCanBubble,
                            bool aCancelable,
                            nsIDOMWindow* aView,
                            const nsAString& aData,
                            const nsAString& aLocale,
                            ErrorResult& aRv)
  {
    aRv = InitCompositionEvent(aType, aCanBubble, aCancelable, aView,
                               aData, aLocale);
  }

protected:
  ~CompositionEvent() {}

  nsString mData;
  nsString mLocale;
};

} 
} 

#endif 
