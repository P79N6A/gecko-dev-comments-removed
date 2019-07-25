





































#include "nsXULTabAccessible.h"

#include "nsAccUtils.h"
#include "nsRelUtils.h"
#include "States.h"


#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULRelatedElement.h"





nsXULTabAccessible::
  nsXULTabAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
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
    nsCOMPtr<nsIDOMXULElement> tab(do_QueryInterface(mContent));
    if ( tab )
    {
      tab->Click();
      return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_INVALID_ARG;
}




PRUint32
nsXULTabAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PAGETAB;
}

PRUint64
nsXULTabAccessible::NativeState()
{
  

  
  PRUint64 state = nsAccessibleWrap::NativeState();

  
  
  
  state &= ~states::FOCUSABLE;

  nsIFrame *frame = mContent->GetPrimaryFrame();
  if (frame) {
    const nsStyleUserInterface* ui = frame->GetStyleUserInterface();
    if (ui->mUserFocus == NS_STYLE_USER_FOCUS_NORMAL)
      state |= states::FOCUSABLE;
  }

  
  state |= states::SELECTABLE;
  state &= ~states::SELECTED;
  nsCOMPtr<nsIDOMXULSelectControlItemElement> tab(do_QueryInterface(mContent));
  if (tab) {
    PRBool selected = PR_FALSE;
    if (NS_SUCCEEDED(tab->GetSelected(&selected)) && selected)
      state |= states::SELECTED;
  }
  return state;
}


NS_IMETHODIMP
nsXULTabAccessible::GetRelationByType(PRUint32 aRelationType,
                                      nsIAccessibleRelation **aRelation)
{
  nsresult rv = nsAccessibleWrap::GetRelationByType(aRelationType,
                                                    aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRelationType != nsIAccessibleRelation::RELATION_LABEL_FOR)
    return NS_OK;

  
  nsCOMPtr<nsIDOMXULRelatedElement> tabsElm =
    do_QueryInterface(mContent->GetParent());
  if (!tabsElm)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> DOMNode(GetDOMNode());
  nsCOMPtr<nsIDOMNode> tabpanelNode;
  tabsElm->GetRelatedElement(DOMNode, getter_AddRefs(tabpanelNode));
  if (!tabpanelNode)
    return NS_OK;

  nsCOMPtr<nsIContent> tabpanelContent(do_QueryInterface(tabpanelNode));
  return nsRelUtils::AddTargetFromContent(aRelationType, aRelation,
                                          tabpanelContent);
}

void
nsXULTabAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                               PRInt32 *aSetSize)
{
  nsAccUtils::GetPositionAndSizeForXULSelectControlItem(mContent, aPosInSet,
                                                        aSetSize);
}






nsXULTabsAccessible::
  nsXULTabsAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULSelectableAccessible(aContent, aShell)
{
}

PRUint32
nsXULTabsAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PAGETABLIST;
}

NS_IMETHODIMP
nsXULTabsAccessible::GetNumActions(PRUint8 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  return NS_OK;
}


NS_IMETHODIMP nsXULTabsAccessible::GetValue(nsAString& _retval)
{
  return NS_OK;
}

nsresult
nsXULTabsAccessible::GetNameInternal(nsAString& aName)
{
  
  return NS_OK;
}






nsXULTabpanelsAccessible::
  nsXULTabpanelsAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXULTabpanelsAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PANE;
}






nsXULTabpanelAccessible::
  nsXULTabpanelAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXULTabpanelAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PROPERTYPAGE;
}

NS_IMETHODIMP
nsXULTabpanelAccessible::GetRelationByType(PRUint32 aRelationType,
                                           nsIAccessibleRelation **aRelation)
{
  nsresult rv = nsAccessibleWrap::GetRelationByType(aRelationType, aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRelationType != nsIAccessibleRelation::RELATION_LABELLED_BY)
    return NS_OK;

  
  nsCOMPtr<nsIDOMXULRelatedElement> tabpanelsElm =
    do_QueryInterface(mContent->GetParent());
  if (!tabpanelsElm)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> DOMNode(GetDOMNode());
  nsCOMPtr<nsIDOMNode> tabNode;
  tabpanelsElm->GetRelatedElement(DOMNode, getter_AddRefs(tabNode));
  if (!tabNode)
    return NS_OK;

  nsCOMPtr<nsIContent> tabContent(do_QueryInterface(tabNode));
  return nsRelUtils::AddTargetFromContent(aRelationType, aRelation,
                                          tabContent);
}
