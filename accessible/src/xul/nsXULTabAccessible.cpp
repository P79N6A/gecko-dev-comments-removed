






































#include "nsXULTabAccessible.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"






nsXULTabAccessible::nsXULTabAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsLeafAccessible(aNode, aShell)
{ 
}


NS_IMETHODIMP nsXULTabAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}


NS_IMETHODIMP nsXULTabAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Switch) {
    aName.AssignLiteral("switch"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsXULTabAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Switch) {
    nsCOMPtr<nsIDOMXULElement> tab(do_QueryInterface(mDOMNode));
    if ( tab )
    {
      tab->Click();
      return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsXULTabAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_PAGETAB;
  return NS_OK;
}




NS_IMETHODIMP nsXULTabAccessible::GetState(PRUint32 *_retval)
{
  
  nsLeafAccessible::GetState(_retval);

  
  
  
  *_retval &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
  if (presShell && content) {
    nsIFrame *frame = presShell->GetPrimaryFrameFor(content);
    if (frame) {
      const nsStyleUserInterface* ui = frame->GetStyleUserInterface();
      if (ui->mUserFocus == NS_STYLE_USER_FOCUS_NORMAL)
        *_retval |= nsIAccessibleStates::STATE_FOCUSABLE;
    }
  }
  
  *_retval |= nsIAccessibleStates::STATE_SELECTABLE;
  *_retval &= ~nsIAccessibleStates::STATE_SELECTED;
  nsCOMPtr<nsIDOMXULSelectControlItemElement> tab(do_QueryInterface(mDOMNode));
  if (tab) {
    PRBool selected = PR_FALSE;
    if (NS_SUCCEEDED(tab->GetSelected(&selected)) && selected)
      *_retval |= nsIAccessibleStates::STATE_SELECTED;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXULTabAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  NS_ENSURE_TRUE(mDOMNode, NS_ERROR_FAILURE);

  nsresult rv = nsLeafAccessible::GetAttributes(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAccessibilityUtils::
    SetAccAttrsForXULSelectControlItem(mDOMNode, *aAttributes);

  return NS_OK;
}








nsXULTabBoxAccessible::nsXULTabBoxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{ 
}


NS_IMETHODIMP nsXULTabBoxAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_PANE;
  return NS_OK;
}


NS_IMETHODIMP nsXULTabBoxAccessible::GetState(PRUint32 *_retval)
{
  nsAccessible::GetState(_retval);
  *_retval &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  return NS_OK;
}

#ifdef NEVER

NS_IMETHODIMP nsXULTabBoxAccessible::GetChildCount(PRInt32 *_retval)
{
  *_retval = 2;
  return NS_OK;
}
#endif















nsXULTabPanelsAccessible::nsXULTabPanelsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{ 
}


NS_IMETHODIMP nsXULTabPanelsAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PROPERTYPAGE;
  return NS_OK;
}




NS_IMETHODIMP nsXULTabPanelsAccessible::GetState(PRUint32 *_retval)
{
  
  nsAccessible::GetState(_retval);
  return NS_OK;
}






NS_IMETHODIMP nsXULTabPanelsAccessible::GetName(nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}






nsXULTabsAccessible::nsXULTabsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsXULSelectableAccessible(aNode, aShell)
{ 
}


NS_IMETHODIMP nsXULTabsAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_PAGETABLIST;
  return NS_OK;
}


NS_IMETHODIMP nsXULTabsAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 0;
  return NS_OK;
}


NS_IMETHODIMP nsXULTabsAccessible::GetState(PRUint32 *_retval)
{
  return nsAccessible::GetState(_retval);
}


NS_IMETHODIMP nsXULTabsAccessible::GetValue(nsAString& _retval)
{
  return NS_OK;
}


NS_IMETHODIMP nsXULTabsAccessible::GetName(nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}
