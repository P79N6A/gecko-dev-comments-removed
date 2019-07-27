




#include "mozilla/dom/BeforeAfterKeyboardEvent.h"
#include "mozilla/TextEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

BeforeAfterKeyboardEvent::BeforeAfterKeyboardEvent(
                                       EventTarget* aOwner,
                                       nsPresContext* aPresContext,
                                       InternalBeforeAfterKeyboardEvent* aEvent)
  : KeyboardEvent(aOwner, aPresContext,
                  aEvent ? aEvent :
                           new InternalBeforeAfterKeyboardEvent(false, 0,
                                                                nullptr))
{
  MOZ_ASSERT(mEvent->mClass == eBeforeAfterKeyboardEventClass,
             "event type mismatch eBeforeAfterKeyboardEventClass");

  if (!aEvent) {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
}


already_AddRefed<BeforeAfterKeyboardEvent>
BeforeAfterKeyboardEvent::Constructor(
                            EventTarget* aOwner,
                            const nsAString& aType,
                            const BeforeAfterKeyboardEventInit& aParam)
{
  nsRefPtr<BeforeAfterKeyboardEvent> event =
    new BeforeAfterKeyboardEvent(aOwner, nullptr, nullptr);
  ErrorResult rv;
  event->InitWithKeyboardEventInit(aOwner, aType, aParam, rv);
  NS_WARN_IF(rv.Failed());

  event->mEvent->AsBeforeAfterKeyboardEvent()->mEmbeddedCancelled =
    aParam.mEmbeddedCancelled;

  return event.forget();
}


already_AddRefed<BeforeAfterKeyboardEvent>
BeforeAfterKeyboardEvent::Constructor(
                            const GlobalObject& aGlobal,
                            const nsAString& aType,
                            const BeforeAfterKeyboardEventInit& aParam,
                            ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> owner = do_QueryInterface(aGlobal.GetAsSupports());
  return Constructor(owner, aType, aParam);
}

Nullable<bool>
BeforeAfterKeyboardEvent::GetEmbeddedCancelled()
{
  nsAutoString type;
  GetType(type);
  if (type.EqualsLiteral("mozbrowserafterkeydown") ||
      type.EqualsLiteral("mozbrowserafterkeyup")) {
    return mEvent->AsBeforeAfterKeyboardEvent()->mEmbeddedCancelled;
  }
  return Nullable<bool>();
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMBeforeAfterKeyboardEvent(nsIDOMEvent** aInstancePtrResult,
                                  EventTarget* aOwner,
                                  nsPresContext* aPresContext,
                                  InternalBeforeAfterKeyboardEvent* aEvent)
{
  BeforeAfterKeyboardEvent* it =
    new BeforeAfterKeyboardEvent(aOwner, aPresContext, aEvent);

  NS_ADDREF(it);
  *aInstancePtrResult = static_cast<Event*>(it);
  return NS_OK;
}
