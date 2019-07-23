





































#include "nsMenuDismissalListener.h"
#include "nsIMenuParent.h"
#include "nsMenuFrame.h"
#include "nsIPopupBoxObject.h"


nsMenuDismissalListener* nsMenuDismissalListener::sInstance = nsnull;





NS_IMPL_ADDREF(nsMenuDismissalListener)
NS_IMPL_RELEASE(nsMenuDismissalListener)
NS_INTERFACE_MAP_BEGIN(nsMenuDismissalListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIMenuRollup)
  NS_INTERFACE_MAP_ENTRY(nsIRollupListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMouseListener)
NS_INTERFACE_MAP_END




nsMenuDismissalListener::nsMenuDismissalListener() :
  mEnabled(PR_TRUE)
{
  mMenuParent = nsnull;
}

nsMenuDismissalListener::~nsMenuDismissalListener() 
{
}

nsMenuDismissalListener*
nsMenuDismissalListener::GetInstance()
{
  if (!sInstance) {
    sInstance = new nsMenuDismissalListener();
    NS_IF_ADDREF(sInstance);
  }
  return sInstance;
}

 void
nsMenuDismissalListener::Shutdown()
{
  if (sInstance) {
    if (sInstance->mMenuParent)
      sInstance->mMenuParent->RemoveKeyboardNavigator();
    sInstance->Unregister();
    NS_RELEASE(sInstance);
  }
}


nsIMenuParent*
nsMenuDismissalListener::GetCurrentMenuParent()
{
  return mMenuParent;
}

void
nsMenuDismissalListener::SetCurrentMenuParent(nsIMenuParent* aMenuParent)
{
  if (aMenuParent == mMenuParent)
    return;

  mMenuParent = aMenuParent;

  if (!aMenuParent) {
    Shutdown();
    return;
  }

  Unregister();
  Register();
}

NS_IMETHODIMP
nsMenuDismissalListener::Rollup()
{
  if (mEnabled) {
    if (mMenuParent) {
      AddRef();
      mMenuParent->HideChain();
      mMenuParent->DismissChain();
      Release();
    }
    else
      Shutdown();
  }
  return NS_OK;
}


NS_IMETHODIMP nsMenuDismissalListener::ShouldRollupOnMouseWheelEvent(PRBool *aShouldRollup) 
{ 
  *aShouldRollup = PR_FALSE; 
  return NS_OK;
}





NS_IMETHODIMP nsMenuDismissalListener::ShouldRollupOnMouseActivate(PRBool *aShouldRollup) 
{ 
  *aShouldRollup = PR_FALSE; 
  return NS_OK;
}

NS_IMETHODIMP
nsMenuDismissalListener::GetSubmenuWidgetChain(nsISupportsArray **_retval)
{
  NS_NewISupportsArray ( _retval );
  nsIMenuParent *curr = mMenuParent;
  while ( curr ) {
    nsCOMPtr<nsIWidget> widget;
    curr->GetWidget ( getter_AddRefs(widget) );
    nsCOMPtr<nsISupports> genericWidget ( do_QueryInterface(widget) );
    (**_retval).AppendElement ( genericWidget );
    
    
    nsIFrame* currAsFrame = nsnull;
    if ( NS_SUCCEEDED(CallQueryInterface(curr, &currAsFrame)) ) {
      nsIMenuFrame *menuFrame = nsnull;
      nsIFrame *parentFrame = currAsFrame->GetParent();
      if (parentFrame) {
        CallQueryInterface(parentFrame, &menuFrame);
      }
      if ( menuFrame ) {
        curr = menuFrame->GetMenuParent ();       
      }
      else {
        
        
        return NS_OK;
      }
    }
    else {
      
      NS_WARNING ( "nsIMenuParent that is not a nsIFrame" );
      return NS_ERROR_FAILURE;
    }
  } 
  
  return NS_OK; 
}


void
nsMenuDismissalListener::Register()
{
  if (mWidget)
    return;

  nsCOMPtr<nsIWidget> widget;
  mMenuParent->GetWidget(getter_AddRefs(widget));
  if (!widget) {
    Shutdown();
    return;
  }

  PRBool consumeOutsideClicks = PR_FALSE;
  mMenuParent->ConsumeOutsideClicks(consumeOutsideClicks);
  widget->CaptureRollupEvents(this, PR_TRUE, consumeOutsideClicks);
  mWidget = widget;

  mMenuParent->AttachedDismissalListener();
}

void
nsMenuDismissalListener::Unregister()
{
  if (mWidget) {
    mWidget->CaptureRollupEvents(this, PR_FALSE, PR_FALSE);
    mWidget = nsnull;
  }
}

void
nsMenuDismissalListener::EnableListener(PRBool aEnabled)
{
  mEnabled = aEnabled;
}

