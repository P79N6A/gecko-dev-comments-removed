




































#include "nsCOMPtr.h"
#include "nsIDocumentViewer.h"
#include "nsIContent.h"
#include "nsPresContext.h"

#include "nsMenuBarX.h"         
#include "nsMenuX.h"
#include "nsMenuItemX.h"
#include "nsMenuItemIcon.h"

#include "nsWidgetAtoms.h"

#include "nsIMenu.h"
#include "nsIMenuBar.h"
#include "nsIWidget.h"
#include "nsIMenuListener.h"
#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMDocumentEvent.h"

#include "nsGUIEvent.h"


#if DEBUG
nsInstanceCounter   gMenuItemCounterX("nsMenuItemX");
#endif


NS_IMPL_ISUPPORTS4(nsMenuItemX, nsIMenuItem, nsIMenuListener, nsIChangeObserver, nsISupportsWeakReference)




nsMenuItemX::nsMenuItemX()
{
  mMenuParent         = nsnull;
  mManager            = nsnull;
  mIsSeparator        = PR_FALSE;
  mKeyEquivalent.AssignLiteral(" ");
  mEnabled            = PR_TRUE;
  mIsChecked          = PR_FALSE;
  mMenuType           = eRegular;

#if DEBUG
  ++gMenuItemCounterX;
#endif 
}




nsMenuItemX::~nsMenuItemX()
{
  if (mManager) {
    if (mContent)
      mManager->Unregister(mContent);
    if (mCommandContent)
      mManager->Unregister(mCommandContent);
  }

#if DEBUG
  --gMenuItemCounterX;
#endif 
}


NS_METHOD nsMenuItemX::Create(nsIMenu* aParent, const nsString & aLabel, PRBool aIsSeparator,
                              EMenuItemType aItemType, nsIChangeManager* aManager,
                              nsIDocShell* aShell, nsIContent* aNode)
{
  mContent = aNode;         
  mMenuParent = aParent;    
  mDocShellWeakRef = do_GetWeakReference(aShell);
  
  mMenuType = aItemType;
  
  
  mManager = aManager;
  nsCOMPtr<nsIChangeObserver> obs = do_QueryInterface(NS_STATIC_CAST(nsIChangeObserver*,this));
  mManager->Register(mContent, obs);   
  
  mEnabled = !mContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::disabled, nsWidgetAtoms::_true, eCaseMatters);
  
  mIsSeparator = aIsSeparator;
  mLabel = aLabel;
  
  
  
  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(aNode->GetDocument());
  if (domDocument) {
    nsAutoString ourCommand;
    aNode->GetAttr(kNameSpaceID_None, nsWidgetAtoms::command, ourCommand);
    if (!ourCommand.IsEmpty()) {
      nsCOMPtr<nsIDOMElement> commandElt;
      domDocument->GetElementById(ourCommand, getter_AddRefs(commandElt));
      if (commandElt) {
        mCommandContent = do_QueryInterface(commandElt);
        mManager->Register(mCommandContent, obs);
      }
    }
  }

  mIcon = new nsMenuItemIcon(NS_STATIC_CAST(nsIMenuItem*, this),
                             mMenuParent, mContent);

  return NS_OK;
}

NS_METHOD
nsMenuItemX::GetLabel(nsString &aText)
{
  aText = mLabel;
  return NS_OK;
}


NS_METHOD 
nsMenuItemX::GetEnabled(PRBool *aIsEnabled)
{
  *aIsEnabled = mEnabled;
  return NS_OK;
}


NS_METHOD nsMenuItemX::SetChecked(PRBool aIsEnabled)
{
  mIsChecked = aIsEnabled;
  
  
  
  if (mIsChecked)
      mContent->SetAttr(kNameSpaceID_None, nsWidgetAtoms::checked,
                        NS_LITERAL_STRING("true"), PR_TRUE);
  else
      mContent->UnsetAttr(kNameSpaceID_None, nsWidgetAtoms::checked, PR_TRUE);

  return NS_OK;
}


NS_METHOD nsMenuItemX::GetChecked(PRBool *aIsEnabled)
{
  *aIsEnabled = mIsChecked;
  return NS_OK;
}



NS_METHOD nsMenuItemX::GetMenuItemType(EMenuItemType *aType)
{
  *aType = mMenuType;
  return NS_OK;
}


NS_METHOD nsMenuItemX::GetNativeData(void *& aData)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD nsMenuItemX::AddMenuListener(nsIMenuListener * aMenuListener)
{
  mXULCommandListener = aMenuListener;    
  return NS_OK;
}


NS_METHOD nsMenuItemX::RemoveMenuListener(nsIMenuListener * aMenuListener)
{
  if (mXULCommandListener.get() == aMenuListener)
    mXULCommandListener = nsnull;
  return NS_OK;
}


NS_METHOD nsMenuItemX::IsSeparator(PRBool & aIsSep)
{
  aIsSep = mIsSeparator;
  return NS_OK;
}





nsEventStatus nsMenuItemX::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  
  return nsEventStatus_eConsumeNoDefault;
}


nsEventStatus nsMenuItemX::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}




nsEventStatus nsMenuItemX::MenuDeselected(const nsMenuEvent & aMenuEvent)
{
    return nsEventStatus_eIgnore;
}


nsEventStatus nsMenuItemX::MenuConstruct(
    const nsMenuEvent & aMenuEvent,
    nsIWidget         * aParentWindow, 
    void              * menuNode,
    void              * aDocShell)
{
    return nsEventStatus_eIgnore;
}


nsEventStatus nsMenuItemX::MenuDestruct(const nsMenuEvent & aMenuEvent)
{
    return nsEventStatus_eIgnore;
}


nsEventStatus nsMenuItemX::CheckRebuild(PRBool & aNeedsRebuild)
{
  aNeedsRebuild = PR_TRUE; 
  return nsEventStatus_eIgnore;
}


nsEventStatus nsMenuItemX::SetRebuild(PRBool aNeedsRebuild)
{
  
  return nsEventStatus_eIgnore;
}






NS_METHOD nsMenuItemX::DoCommand()
{
    
    if (mMenuType == nsIMenuItem::eCheckbox || (mMenuType == nsIMenuItem::eRadio && !mIsChecked)) {
        if (!mContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::autocheck,
                                   nsWidgetAtoms::_false, eCaseMatters))
            SetChecked(!mIsChecked);
            
    }

    return MenuHelpersX::DispatchCommandTo(mDocShellWeakRef, mContent);
}


NS_IMETHODIMP nsMenuItemX::DispatchDOMEvent(const nsString &eventName, PRBool *preventDefaultCalled)
{
  if (!mContent)
    return NS_ERROR_FAILURE;
  
  
  nsCOMPtr<nsIDocument> parentDoc = mContent->GetOwnerDoc();
  if (!parentDoc) {
    NS_WARNING("Failed to get owner nsIDocument for menu item content");
    return NS_ERROR_FAILURE;
  }
  
  
  nsCOMPtr<nsIDOMDocumentEvent> DOMEventFactory = do_QueryInterface(parentDoc);
  if (!DOMEventFactory) {
    NS_WARNING("Failed to QI parent nsIDocument to nsIDOMDocumentEvent");
    return NS_ERROR_FAILURE;
  }
  
  
  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = DOMEventFactory->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create nsIDOMEvent");
    return rv;
  }
  event->InitEvent(eventName, PR_TRUE, PR_TRUE);
  
  
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
  privateEvent->SetTrusted(PR_TRUE);
  
  
  nsCOMPtr<nsIDOMEventTarget> eventTarget = do_QueryInterface(mContent);
  rv = eventTarget->DispatchEvent(event, preventDefaultCalled);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to send DOM event via nsIDOMEventTarget");
    return rv;
  }
  
  return NS_OK;
}

   

NS_METHOD nsMenuItemX::GetModifiers(PRUint8 * aModifiers) 
{
    *aModifiers = mModifiers; 
    return NS_OK; 
}


NS_METHOD nsMenuItemX::SetModifiers(PRUint8 aModifiers)
{
    mModifiers = aModifiers;
    return NS_OK;
}
 

NS_METHOD nsMenuItemX::SetShortcutChar(const nsString &aText)
{
    mKeyEquivalent = aText;
    return NS_OK;
} 


NS_METHOD nsMenuItemX::GetShortcutChar(nsString &aText)
{
    aText = mKeyEquivalent;
    return NS_OK;
} 







void
nsMenuItemX :: UncheckRadioSiblings(nsIContent* inCheckedContent)
{
  nsAutoString myGroupName;
  inCheckedContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::name, myGroupName);
  if ( ! myGroupName.Length() )        
    return;
  
  nsCOMPtr<nsIContent> parent = inCheckedContent->GetParent();
  if ( !parent )
    return;

  
  PRUint32 count = parent->GetChildCount();
  for ( PRUint32 i = 0; i < count; ++i ) {
    nsIContent *sibling = parent->GetChildAt(i);
    if ( sibling ) {      
      if ( sibling != inCheckedContent ) {                    
        
        if (sibling->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::name,
                                 myGroupName, eCaseMatters))
          sibling->SetAttr(kNameSpaceID_None, nsWidgetAtoms::checked, NS_LITERAL_STRING("false"), PR_TRUE);
      }
    }    
  } 

} 

#pragma mark -






NS_IMETHODIMP
nsMenuItemX::AttributeChanged(nsIDocument *aDocument, PRInt32 aNameSpaceID, nsIContent *aContent, nsIAtom *aAttribute)
{
  if (aContent == mContent) {
    if (aAttribute == nsWidgetAtoms::checked) {
      
      
      if (mMenuType == eRadio) {
        if (mContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::checked,
                                  nsWidgetAtoms::_true, eCaseMatters))
          UncheckRadioSiblings(mContent);
      }
      
      nsCOMPtr<nsIMenuListener> listener = do_QueryInterface(mMenuParent);
      listener->SetRebuild(PR_TRUE);
    } 
    else if (aAttribute == nsWidgetAtoms::disabled || aAttribute == nsWidgetAtoms::hidden ||
             aAttribute == nsWidgetAtoms::collapsed || aAttribute == nsWidgetAtoms::label )  {
      nsCOMPtr<nsIMenuListener> listener = do_QueryInterface(mMenuParent);
      listener->SetRebuild(PR_TRUE);
    }    
    else if (aAttribute == nsWidgetAtoms::image) {
      SetupIcon();
    }
  }
  else if (aContent == mCommandContent &&
    aAttribute == nsWidgetAtoms::disabled &&
    mMenuParent && mCommandContent) {
    nsAutoString menuItemDisabled;
    nsAutoString commandDisabled;
    mContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, menuItemDisabled);
    mCommandContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, commandDisabled);
    if (!commandDisabled.Equals(menuItemDisabled)) {
      
      if (commandDisabled.IsEmpty())
        mContent->UnsetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, PR_TRUE);
      else
        mContent->SetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, commandDisabled, PR_TRUE);
    }
    
    mMenuParent->ChangeNativeEnabledStatusForMenuItem(this, !commandDisabled.EqualsLiteral("true"));
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemX :: ContentRemoved(nsIDocument *aDocument, nsIContent *aChild, PRInt32 aIndexInContainer)
{
  if (aChild == mCommandContent) {
    mManager->Unregister(mCommandContent);
    mCommandContent = nsnull;
  }
  
  nsCOMPtr<nsIMenuListener> listener = do_QueryInterface(mMenuParent);
  listener->SetRebuild(PR_TRUE);
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemX :: ContentInserted(nsIDocument *aDocument, nsIContent *aChild, PRInt32 aIndexInContainer)
{
  
  nsCOMPtr<nsIMenuListener> listener = do_QueryInterface(mMenuParent);
  listener->SetRebuild(PR_TRUE);
  return NS_OK;
  
} 


NS_IMETHODIMP
nsMenuItemX::SetupIcon()
{
  if (!mIcon) return NS_ERROR_OUT_OF_MEMORY;

  return mIcon->SetupIcon();
}
