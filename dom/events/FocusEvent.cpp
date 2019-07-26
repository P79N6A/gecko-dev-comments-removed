




#include "mozilla/dom/FocusEvent.h"
#include "mozilla/ContentEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED(FocusEvent, UIEvent, nsIDOMFocusEvent)

FocusEvent::FocusEvent(EventTarget* aOwner,
                       nsPresContext* aPresContext,
                       InternalFocusEvent* aEvent)
  : UIEvent(aOwner, aPresContext,
            aEvent ? aEvent : new InternalFocusEvent(false, NS_FOCUS_CONTENT))
{
  if (aEvent) {
    mEventIsInternal = false;
  } else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
}


NS_IMETHODIMP
FocusEvent::GetRelatedTarget(nsIDOMEventTarget** aRelatedTarget)
{
  NS_ENSURE_ARG_POINTER(aRelatedTarget);
  NS_IF_ADDREF(*aRelatedTarget = GetRelatedTarget());
  return NS_OK;
}

EventTarget*
FocusEvent::GetRelatedTarget()
{
  return mEvent->AsFocusEvent()->relatedTarget;
}

nsresult
FocusEvent::InitFocusEvent(const nsAString& aType,
                           bool aCanBubble,
                           bool aCancelable,
                           nsIDOMWindow* aView,
                           int32_t aDetail,
                           EventTarget* aRelatedTarget)
{
  nsresult rv =
    UIEvent::InitUIEvent(aType, aCanBubble, aCancelable, aView, aDetail);
  NS_ENSURE_SUCCESS(rv, rv);
  mEvent->AsFocusEvent()->relatedTarget = aRelatedTarget;
  return NS_OK;
}

already_AddRefed<FocusEvent>
FocusEvent::Constructor(const GlobalObject& aGlobal,
                        const nsAString& aType,
                        const FocusEventInit& aParam,
                        ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<FocusEvent> e = new FocusEvent(t, nullptr, nullptr);
  bool trusted = e->Init(t);
  aRv = e->InitFocusEvent(aType, aParam.mBubbles, aParam.mCancelable, aParam.mView,
                          aParam.mDetail, aParam.mRelatedTarget);
  e->SetTrusted(trusted);
  return e.forget();
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMFocusEvent(nsIDOMEvent** aInstancePtrResult,
                    EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    InternalFocusEvent* aEvent)
{
  FocusEvent* it = new FocusEvent(aOwner, aPresContext, aEvent);
  NS_ADDREF(it);
  *aInstancePtrResult = static_cast<Event*>(it);
  return NS_OK;
}
