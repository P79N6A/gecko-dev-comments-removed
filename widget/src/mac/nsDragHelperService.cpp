




































#include "nsDragHelperService.h"

#include "nsGUIEvent.h"
#include "nsIDOMNode.h"
#include "nsIDragSessionMac.h"
#include "nsIServiceManager.h"

#define kDragServiceContractID "@mozilla.org/widget/dragservice;1"


NS_IMPL_ADDREF(nsDragHelperService)
NS_IMPL_RELEASE(nsDragHelperService)
NS_IMPL_QUERY_INTERFACE1(nsDragHelperService, nsIDragHelperService)





nsDragHelperService::nsDragHelperService()
: mDragOverTimer(nsnull)
, mDragOverDragRef(nsnull)
{
}





nsDragHelperService::~nsDragHelperService()
{
  if (mDragOverTimer)
    ::RemoveEventLoopTimer(mDragOverTimer);

  NS_ASSERTION(!mDragService.get(),
               "A drag was not correctly ended by shutdown");
}










NS_IMETHODIMP
nsDragHelperService::Enter(DragReference inDragRef, nsIEventSink *inSink)
{
  
  mDragService = do_GetService(kDragServiceContractID);
  NS_ASSERTION(mDragService,
               "Couldn't get a drag service, we're in biiig trouble");
  if (!mDragService || !inSink)
    return NS_ERROR_FAILURE;

  
  mDragService->StartDragSession();
  nsCOMPtr<nsIDragSessionMac> macSession(do_QueryInterface(mDragService));
  if (macSession)
    macSession->SetDragReference(inDragRef);

  DoDragAction(NS_DRAGDROP_ENTER, inDragRef, inSink);

  
  SetDragOverTimer(inSink, inDragRef);

  return NS_OK;
}










NS_IMETHODIMP
nsDragHelperService::Tracking(DragReference inDragRef, nsIEventSink *inSink,
                              PRBool* outDropAllowed)
{
  NS_ASSERTION(mDragService,
               "Couldn't get a drag service, we're in biiig trouble");
  if (!mDragService || !inSink) {
    *outDropAllowed = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  
  
  nsCOMPtr<nsIDragSession> session;
  mDragService->GetCurrentSession(getter_AddRefs(session));
  NS_ASSERTION(session, "If we don't have a drag session, we're fucked");
  if (session)
    session->SetCanDrop(PR_FALSE);

  
  
  SetDragOverTimer(inSink, inDragRef);

  DoDragAction(NS_DRAGDROP_OVER, inDragRef, inSink);

  
  if (session)
    session->GetCanDrop(outDropAllowed);

  return NS_OK;
}








NS_IMETHODIMP
nsDragHelperService::Leave(DragReference inDragRef, nsIEventSink *inSink)
{
  
  SetDragOverTimer(nsnull, nsnull);

  NS_ASSERTION(mDragService,
               "Couldn't get a drag service, we're in biiig trouble");
  if (!mDragService || !inSink)
    return NS_ERROR_FAILURE;

  
  
  
  
  nsCOMPtr<nsIDragSessionMac> macSession(do_QueryInterface(mDragService));
  if (macSession)
    macSession->SetDragReference(0);

  DoDragAction(NS_DRAGDROP_EXIT, inDragRef, inSink);

#ifndef MOZ_WIDGET_COCOA
  ::HideDragHilite(inDragRef);
#endif

  nsCOMPtr<nsIDragSession> currentDragSession;
  mDragService->GetCurrentSession(getter_AddRefs(currentDragSession));

  if (currentDragSession) {
    nsCOMPtr<nsIDOMNode> sourceNode;
    currentDragSession->GetSourceNode(getter_AddRefs(sourceNode));

    if (!sourceNode) {
      
      
      
      
      mDragService->EndDragSession(PR_FALSE);
    }
  }

  
  mDragService = nsnull;

  return NS_OK;
}








NS_IMETHODIMP
nsDragHelperService::Drop(DragReference inDragRef, nsIEventSink *inSink,
                          PRBool* outAccepted)
{
  
  SetDragOverTimer(nsnull, nsnull);

  NS_ASSERTION(mDragService,
               "Couldn't get a drag service, we're in biiig trouble");
  if (!mDragService || !inSink) {
    *outAccepted = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  OSErr result = noErr;
  nsCOMPtr<nsIDragSession> dragSession;
  mDragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (dragSession) {
    
    
    PRBool canDrop = PR_FALSE;
    if (NS_SUCCEEDED(dragSession->GetCanDrop(&canDrop)))
      if (canDrop)
        DoDragAction(NS_DRAGDROP_DROP, inDragRef, inSink);
      else
        result = dragNotAcceptedErr;
  } 

  
  *outAccepted = (result == noErr);

  return NS_OK;
}








PRBool
nsDragHelperService::DoDragAction(PRUint32      aMessage,
                                  DragReference aDragRef,
                                  nsIEventSink* aSink)
{
  
  Point mouseLocGlobal;
  ::GetDragMouse(aDragRef, &mouseLocGlobal, nsnull);
  short modifiers;
  ::GetDragModifiers(aDragRef, &modifiers, nsnull, nsnull);

  
  SetDragActionBasedOnModifiers(modifiers);

  
  PRBool handled = PR_FALSE;
  aSink->DragEvent(aMessage, mouseLocGlobal.h, mouseLocGlobal.v,
                   modifiers, &handled);

  return handled;
}








void
nsDragHelperService::SetDragActionBasedOnModifiers(short inModifiers)
{
  nsCOMPtr<nsIDragSession> dragSession;
  mDragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (dragSession) {
    PRUint32 action = nsIDragService::DRAGDROP_ACTION_MOVE;

    
    if (inModifiers & optionKey) {
      if (inModifiers & cmdKey)
        action = nsIDragService::DRAGDROP_ACTION_LINK;
      else
        action = nsIDragService::DRAGDROP_ACTION_COPY;
    }

    dragSession->SetDragAction(action);
  }

} 









void
nsDragHelperService::SetDragOverTimer(nsIEventSink* aSink,
                                      DragRef       aDragRef)
{
  
  const EventTimerInterval kDragOverInterval = 1.0/10.0;

  if (!aSink || !aDragRef) {
    
    if (mDragOverTimer) {
      ::RemoveEventLoopTimer(mDragOverTimer);
      mDragOverTimer = nsnull;
      mDragOverSink = nsnull;
    }
    return;
  }

  if (mDragOverTimer) {
    
    ::SetEventLoopTimerNextFireTime(mDragOverTimer, kDragOverInterval);
    return;
  }

  mDragOverSink = aSink;
  mDragOverDragRef = aDragRef;

  static EventLoopTimerUPP sDragOverTimerUPP;
  if (!sDragOverTimerUPP)
    sDragOverTimerUPP = ::NewEventLoopTimerUPP(DragOverTimerHandler);
 
  ::InstallEventLoopTimer(::GetCurrentEventLoop(),
                          kDragOverInterval,
                          kDragOverInterval,
                          sDragOverTimerUPP,
                          this,
                          &mDragOverTimer);
}







void
nsDragHelperService::DragOverTimerHandler(EventLoopTimerRef aTimer,
                                          void* aUserData)
{
  nsDragHelperService* self = static_cast<nsDragHelperService*>(aUserData);

  self->DoDragAction(NS_DRAGDROP_OVER,
                     self->mDragOverDragRef, self->mDragOverSink);
}
