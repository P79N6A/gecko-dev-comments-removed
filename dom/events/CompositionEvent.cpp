





#include "mozilla/dom/CompositionEvent.h"
#include "mozilla/TextEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

CompositionEvent::CompositionEvent(EventTarget* aOwner,
                                   nsPresContext* aPresContext,
                                   WidgetCompositionEvent* aEvent)
  : UIEvent(aOwner, aPresContext,
            aEvent ? aEvent : new WidgetCompositionEvent(false, 0, nullptr))
{
  NS_ASSERTION(mEvent->mClass == eCompositionEventClass,
               "event type mismatch");

  if (aEvent) {
    mEventIsInternal = false;
  } else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();

    
    
    
    mEvent->mFlags.mCancelable = false;
  }

  mData = mEvent->AsCompositionEvent()->data;
  
}

NS_IMPL_ADDREF_INHERITED(CompositionEvent, UIEvent)
NS_IMPL_RELEASE_INHERITED(CompositionEvent, UIEvent)

NS_INTERFACE_MAP_BEGIN(CompositionEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCompositionEvent)
NS_INTERFACE_MAP_END_INHERITING(UIEvent)

NS_IMETHODIMP
CompositionEvent::GetData(nsAString& aData)
{
  aData = mData;
  return NS_OK;
}

NS_IMETHODIMP
CompositionEvent::GetLocale(nsAString& aLocale)
{
  aLocale = mLocale;
  return NS_OK;
}

NS_IMETHODIMP
CompositionEvent::InitCompositionEvent(const nsAString& aType,
                                       bool aCanBubble,
                                       bool aCancelable,
                                       nsIDOMWindow* aView,
                                       const nsAString& aData,
                                       const nsAString& aLocale)
{
  nsresult rv = UIEvent::InitUIEvent(aType, aCanBubble, aCancelable, aView, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  mData = aData;
  mLocale = aLocale;
  return NS_OK;
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMCompositionEvent(nsIDOMEvent** aInstancePtrResult,
                          EventTarget* aOwner,
                          nsPresContext* aPresContext,
                          WidgetCompositionEvent* aEvent)
{
  CompositionEvent* event = new CompositionEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(event, aInstancePtrResult);
}
