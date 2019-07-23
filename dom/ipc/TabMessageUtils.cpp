





































#include "TabMessageUtils.h"
#include "nsCOMPtr.h"
#include "nsIDOMEvent.h"
#include "nsEventDispatcher.h"

namespace mozilla {
namespace dom {

bool
ReadRemoteEvent(const IPC::Message* aMsg, void** aIter,
                RemoteDOMEvent* aResult)
{
  aResult->mEvent = nsnull;
  nsString type;
  NS_ENSURE_TRUE(ReadParam(aMsg, aIter, &type), false);

  nsCOMPtr<nsIDOMEvent> event;
  nsEventDispatcher::CreateEvent(nsnull, nsnull, type, getter_AddRefs(event));
  aResult->mEvent = do_QueryInterface(event);
  NS_ENSURE_TRUE(aResult->mEvent, false);

  return aResult->mEvent->Deserialize(aMsg, aIter);
}

}
}
