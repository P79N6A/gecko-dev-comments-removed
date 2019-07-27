




#include "mozilla/dom/InputEvent.h"
#include "mozilla/TextEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

InputEvent::InputEvent(EventTarget* aOwner,
                       nsPresContext* aPresContext,
                       InternalEditorInputEvent* aEvent)
  : UIEvent(aOwner, aPresContext,
            aEvent ? aEvent : new InternalEditorInputEvent(false, 0, nullptr))
{
  NS_ASSERTION(mEvent->mClass == eEditorInputEventClass,
               "event type mismatch");

  if (aEvent) {
    mEventIsInternal = false;
  } else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
}

NS_IMPL_ADDREF_INHERITED(InputEvent, UIEvent)
NS_IMPL_RELEASE_INHERITED(InputEvent, UIEvent)

NS_INTERFACE_MAP_BEGIN(InputEvent)
NS_INTERFACE_MAP_END_INHERITING(UIEvent)

bool
InputEvent::IsComposing()
{
  return mEvent->AsEditorInputEvent()->mIsComposing;
}

already_AddRefed<InputEvent>
InputEvent::Constructor(const GlobalObject& aGlobal,
                        const nsAString& aType,
                        const InputEventInit& aParam,
                        ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<InputEvent> e = new InputEvent(t, nullptr, nullptr);
  bool trusted = e->Init(t);
  aRv = e->InitUIEvent(aType, aParam.mBubbles, aParam.mCancelable,
                       aParam.mView, aParam.mDetail);
  InternalEditorInputEvent* internalEvent = e->mEvent->AsEditorInputEvent();
  internalEvent->mIsComposing = aParam.mIsComposing;
  e->SetTrusted(trusted);
  return e.forget();
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMInputEvent(nsIDOMEvent** aInstancePtrResult,
                    EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    InternalEditorInputEvent* aEvent)
{
  InputEvent* it = new InputEvent(aOwner, aPresContext, aEvent);
  NS_ADDREF(it);
  *aInstancePtrResult = static_cast<Event*>(it);
  return NS_OK;
}
