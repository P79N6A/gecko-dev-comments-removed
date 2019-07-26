




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
                             ErrorResult& rv)
{
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIAtom> type = do_GetAtom(aType);
    return SetEventHandler(type, EmptyString(), aHandler, rv);
  }
  return SetEventHandler(nullptr,
                         Substring(aType, 2), 
                         aHandler, rv);
}

void
EventTarget::SetEventHandler(nsIAtom* aType, const nsAString& aTypeString,
                             EventHandlerNonNull* aHandler,
                             ErrorResult& rv)
{
  rv = GetListenerManager(true)->SetEventHandler(aType,
                                                 aTypeString,
                                                 aHandler);
}

} 
} 
