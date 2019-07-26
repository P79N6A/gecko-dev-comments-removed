




#include "mozilla/dom/EventTarget.h"
#include "nsEventListenerManager.h"


namespace mozilla {
namespace dom {

void
EventTarget::RemoveEventListener(const nsAString& aType,
                                 nsIDOMEventListener* aListener,
                                 bool aUseCapture,
                                 ErrorResult& aRv)
{
  nsEventListenerManager* elm = GetListenerManager(false);
  if (elm) {
    elm->RemoveEventListener(aType, aListener, aUseCapture);
  }
}

EventHandlerNonNull*
EventTarget::GetEventHandler(nsIAtom* aType)
{
  nsEventListenerManager* elm = GetListenerManager(false);
  return elm ? elm->GetEventHandler(aType) : nullptr;
}

void
EventTarget::SetEventHandler(nsIAtom* aType, EventHandlerNonNull* aHandler,
                             ErrorResult& rv)
{
  rv = GetListenerManager(true)->SetEventHandler(aType, aHandler);
}

} 
} 
