





#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/TabMessageUtils.h"
#include "nsCOMPtr.h"
#include "nsIDOMEvent.h"

namespace mozilla {
namespace dom {

bool
ReadRemoteEvent(const IPC::Message* aMsg, void** aIter,
                RemoteDOMEvent* aResult)
{
  aResult->mEvent = nullptr;
  nsString type;
  NS_ENSURE_TRUE(ReadParam(aMsg, aIter, &type), false);

  nsCOMPtr<nsIDOMEvent> event;
  EventDispatcher::CreateEvent(nullptr, nullptr, nullptr, type,
                               getter_AddRefs(event));
  aResult->mEvent = do_QueryInterface(event);
  NS_ENSURE_TRUE(aResult->mEvent, false);

  return aResult->mEvent->Deserialize(aMsg, aIter);
}

} 
} 
