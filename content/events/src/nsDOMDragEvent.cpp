




































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
nsDOMDragEvent::InitDragEventNS(const nsAString & aNamespaceURIArg,
                                const nsAString & aType,
                                PRBool aCanBubble, PRBool aCancelable,
                                nsIDOMAbstractView* aView, PRInt32 aDetail,
                                PRInt32 aScreenX, PRInt32 aScreenY,
                                PRInt32 aClientX, PRInt32 aClientY, 
                                PRBool aCtrlKey, PRBool aAltKey, PRBool aShiftKey,
                                PRBool aMetaKey, PRUint16 aButton,
                                nsIDOMEventTarget *aRelatedTarget,
                                nsIDOMDataTransfer* aDataTransfer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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
  if (dragEvent->dataTransfer) {
    CallQueryInterface(dragEvent->dataTransfer, aDataTransfer);
    return NS_OK;
  }

  
  if (mEventIsInternal) {
    NS_IF_ADDREF(*aDataTransfer = dragEvent->dataTransfer);
    return NS_OK;
  }

  
  
  
  NS_ASSERTION(mEvent->message != NS_DRAGDROP_GESTURE &&
               mEvent->message != NS_DRAGDROP_START,
               "draggesture event created without a dataTransfer");

  nsCOMPtr<nsIDragSession> dragSession = nsContentUtils::GetDragSession();
  NS_ENSURE_TRUE(dragSession, NS_OK); 

  nsCOMPtr<nsIDOMDataTransfer> initialDataTransfer;
  dragSession->GetDataTransfer(getter_AddRefs(initialDataTransfer));
  if (!initialDataTransfer) {
    
    
    
    
    
    PRUint32 action = 0;
    dragSession->GetDragAction(&action);
    initialDataTransfer =
      new nsDOMDataTransfer(mEvent->message, action);
    NS_ENSURE_TRUE(initialDataTransfer, NS_ERROR_OUT_OF_MEMORY);

    
    dragSession->SetDataTransfer(initialDataTransfer);
  }

  
  nsCOMPtr<nsIDOMNSDataTransfer> initialDataTransferNS =
    do_QueryInterface(initialDataTransfer);
  NS_ENSURE_TRUE(initialDataTransferNS, NS_ERROR_FAILURE);
  initialDataTransferNS->Clone(mEvent->message,
                               getter_AddRefs(dragEvent->dataTransfer));
  NS_ENSURE_TRUE(dragEvent->dataTransfer, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  if (mEvent->message == NS_DRAGDROP_ENTER ||
      mEvent->message == NS_DRAGDROP_OVER) {
    nsCOMPtr<nsIDOMNSDataTransfer> newDataTransfer =
      do_QueryInterface(dragEvent->dataTransfer);
    NS_ENSURE_TRUE(newDataTransfer, NS_ERROR_FAILURE);

    PRUint32 action, effectAllowed;
    dragSession->GetDragAction(&action);
    newDataTransfer->GetEffectAllowedInt(&effectAllowed);
    newDataTransfer->SetDropEffectInt(FilterDropEffect(action, effectAllowed));
  }
  else if (mEvent->message == NS_DRAGDROP_DROP ||
           mEvent->message == NS_DRAGDROP_DRAGDROP ||
           mEvent->message == NS_DRAGDROP_END) {
    
    
    
    
    nsCOMPtr<nsIDOMNSDataTransfer> newDataTransfer =
      do_QueryInterface(dragEvent->dataTransfer);
    NS_ENSURE_TRUE(newDataTransfer, NS_ERROR_FAILURE);

    PRUint32 dropEffect;
    initialDataTransferNS->GetDropEffectInt(&dropEffect);
    newDataTransfer->SetDropEffectInt(dropEffect);
  }

  NS_IF_ADDREF(*aDataTransfer = dragEvent->dataTransfer);
  return NS_OK;
}


PRUint32
nsDOMDragEvent::FilterDropEffect(PRUint32 aAction, PRUint32 aEffectAllowed)
{
  
  
  
  
  
  if (aAction & nsIDragService::DRAGDROP_ACTION_COPY)
    aAction = nsIDragService::DRAGDROP_ACTION_COPY;
  else if (aAction & nsIDragService::DRAGDROP_ACTION_LINK)
    aAction = nsIDragService::DRAGDROP_ACTION_LINK;
  else if (aAction & nsIDragService::DRAGDROP_ACTION_MOVE)
    aAction = nsIDragService::DRAGDROP_ACTION_MOVE;

  
  
  
  
  
  if (aAction & aEffectAllowed ||
      aEffectAllowed == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED)
    return aAction;
  if (aEffectAllowed & nsIDragService::DRAGDROP_ACTION_MOVE)
    return nsIDragService::DRAGDROP_ACTION_MOVE;
  if (aEffectAllowed & nsIDragService::DRAGDROP_ACTION_COPY)
    return nsIDragService::DRAGDROP_ACTION_COPY;
  if (aEffectAllowed & nsIDragService::DRAGDROP_ACTION_LINK)
    return nsIDragService::DRAGDROP_ACTION_LINK;
  return nsIDragService::DRAGDROP_ACTION_NONE;
}

nsresult NS_NewDOMDragEvent(nsIDOMEvent** aInstancePtrResult,
                            nsPresContext* aPresContext,
                            nsDragEvent *aEvent) 
{
  nsDOMDragEvent* event = new nsDOMDragEvent(aPresContext, aEvent);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(event, aInstancePtrResult);
}
