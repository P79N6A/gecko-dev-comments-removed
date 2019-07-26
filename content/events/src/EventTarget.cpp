




#include "mozilla/dom/EventTarget.h"
#include "nsEventListenerManager.h"
#include "nsThreadUtils.h"

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
EventTarget::GetEventHandler(nsIAtom* aType, const nsAString& aTypeString)
{
  nsEventListenerManager* elm = GetListenerManager(false);
  return elm ? elm->GetEventHandler(aType, aTypeString) : nullptr;
}

void
EventTarget::SetEventHandler(const nsAString& aType,
                             EventHandlerNonNull* aHandler,
                             ErrorResult& aRv)
{
  if (!StringBeginsWith(aType, NS_LITERAL_STRING("on"))) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return;
  }
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIAtom> type = do_GetAtom(aType);
    SetEventHandler(type, EmptyString(), aHandler);
    return;
  }
  SetEventHandler(nullptr,
                  Substring(aType, 2), 
                  aHandler);
}

void
EventTarget::SetEventHandler(nsIAtom* aType, const nsAString& aTypeString,
                             EventHandlerNonNull* aHandler)
{
  GetListenerManager(true)->SetEventHandler(aType, aTypeString, aHandler);
}

} 
} 
