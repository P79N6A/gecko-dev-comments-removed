




#include "CellBroadcast.h"
#include "nsDOMClassInfo.h"

DOMCI_DATA(MozCellBroadcast, mozilla::dom::CellBroadcast)

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(CellBroadcast)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(CellBroadcast,
                                                  nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(CellBroadcast,
                                                nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(CellBroadcast)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozCellBroadcast)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMozCellBroadcast)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozCellBroadcast)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(CellBroadcast, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(CellBroadcast, nsDOMEventTargetHelper)

CellBroadcast::CellBroadcast(nsPIDOMWindow *aWindow)
{
  BindToOwner(aWindow);
}

NS_IMPL_EVENT_HANDLER(CellBroadcast, received)

} 
} 

nsresult
NS_NewCellBroadcast(nsPIDOMWindow* aWindow,
                    nsIDOMMozCellBroadcast** aCellBroadcast)
{
  nsPIDOMWindow* innerWindow = aWindow->IsInnerWindow() ?
    aWindow :
    aWindow->GetCurrentInnerWindow();

  nsRefPtr<mozilla::dom::CellBroadcast> cb =
    new mozilla::dom::CellBroadcast(innerWindow);
  cb.forget(aCellBroadcast);

  return NS_OK;
}
