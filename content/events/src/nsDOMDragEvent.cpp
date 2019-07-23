




































#include "nsDOMDragEvent.h"
#include "nsIServiceManager.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"
#include "nsIEventStateManager.h"
#include "nsDOMDataTransfer.h"
#include "nsIDragService.h"

nsDOMDragEvent::nsDOMDragEvent(nsPresContext* aPresContext,
                               nsInputEvent* aEvent)
  : nsDOMMouseEvent(aPresContext, aEvent ? aEvent :
                    new nsDragEvent(PR_FALSE, 0, nsnull))
{
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
    mEvent->refPoint.x = mEvent->refPoint.y = 0;
  }
}

nsDOMDragEvent::~nsDOMDragEvent()
{
  if (mEventIsInternal) {
    if (mEvent->eventStructType == NS_DRAG_EVENT)
      delete static_cast<nsDragEvent*>(mEvent);
    mEvent = nsnull;
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMDragEvent, nsDOMMouseEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMDragEvent, nsDOMMouseEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMDragEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDragEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DragEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMMouseEvent)

NS_IMETHODIMP
nsDOMDragEvent::InitDragEvent(const nsAString & aType,
                              PRBool aCanBubble, PRBool aCancelable,
                              nsIDOMAbstractView* aView, PRInt32 aDetail,
                              PRInt32 aScreenX, PRInt32 aScreenY,
                              PRInt32 aClientX, PRInt32 aClientY, 
                              PRBool aCtrlKey, PRBool aAltKey, PRBool aShiftKey,
                              PRBool aMetaKey, PRUint16 aButton,
                              nsIDOMEventTarget *aRelatedTarget,
                              nsIDOMDataTransfer* aDataTransfer)
{
  nsresult rv = nsDOMMouseEvent::InitMouseEvent(aType, aCanBubble, aCancelable,
                  aView, aDetail, aScreenX, aScreenY, aClientX, aClientY,
                  aCtrlKey, aAltKey, aShiftKey, aMetaKey, aButton,
                  aRelatedTarget);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mEventIsInternal && mEvent) {
    nsDragEvent* dragEvent = static_cast<nsDragEvent*>(mEvent);
    dragEvent->dataTransfer = aDataTransfer;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDragEvent::GetDataTransfer(nsIDOMDataTransfer** aDataTransfer)
{
  
  
  
  
  *aDataTransfer = nsnull;

  if (!mEvent || mEvent->eventStructType != NS_DRAG_EVENT) {
    NS_WARNING("Tried to get dataTransfer from non-drag event!");
    return NS_OK;
  }

  nsDragEvent* dragEvent = static_cast<nsDragEvent*>(mEvent);
  
  if (!mEventIsInternal) {
    nsresult rv = nsContentUtils::SetDataTransferInEvent(dragEvent);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_IF_ADDREF(*aDataTransfer = dragEvent->dataTransfer);
  return NS_OK;
}

nsresult NS_NewDOMDragEvent(nsIDOMEvent** aInstancePtrResult,
                            nsPresContext* aPresContext,
                            nsDragEvent *aEvent) 
{
  nsDOMDragEvent* event = new nsDOMDragEvent(aPresContext, aEvent);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(event, aInstancePtrResult);
}
