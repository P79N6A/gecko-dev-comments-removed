





































#include "nsXULTabAccessible.h"

#include "nsAccUtils.h"
#include "Relation.h"
#include "States.h"


#include "nsIAccessibleRelation.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULRelatedElement.h"

using namespace mozilla::a11y;





nsXULTabAccessible::
  nsXULTabAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}




PRUint8
nsXULTabAccessible::ActionCount()
{
  return 1;
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
    bool selected = false;
    if (NS_SUCCEEDED(tab->GetSelected(&selected)) && selected)
      state |= states::SELECTED;
  }
  return state;
}


Relation
nsXULTabAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = nsAccessibleWrap::RelationByType(aType);
  if (aType != nsIAccessibleRelation::RELATION_LABEL_FOR)
    return rel;

  
  nsCOMPtr<nsIDOMXULRelatedElement> tabsElm =
    do_QueryInterface(mContent->GetParent());
  if (!tabsElm)
    return rel;

  nsCOMPtr<nsIDOMNode> DOMNode(GetDOMNode());
  nsCOMPtr<nsIDOMNode> tabpanelNode;
  tabsElm->GetRelatedElement(DOMNode, getter_AddRefs(tabpanelNode));
  if (!tabpanelNode)
    return rel;

  nsCOMPtr<nsIContent> tabpanelContent(do_QueryInterface(tabpanelNode));
  rel.AppendTarget(tabpanelContent);
  return rel;
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

PRUint8
nsXULTabsAccessible::ActionCount()
{
  return 0;
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

Relation
nsXULTabpanelAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = nsAccessibleWrap::RelationByType(aType);
  if (aType != nsIAccessibleRelation::RELATION_LABELLED_BY)
    return rel;

  
  nsCOMPtr<nsIDOMXULRelatedElement> tabpanelsElm =
    do_QueryInterface(mContent->GetParent());
  if (!tabpanelsElm)
    return rel;

  nsCOMPtr<nsIDOMNode> DOMNode(GetDOMNode());
  nsCOMPtr<nsIDOMNode> tabNode;
  tabpanelsElm->GetRelatedElement(DOMNode, getter_AddRefs(tabNode));
  if (!tabNode)
    return rel;

  nsCOMPtr<nsIContent> tabContent(do_QueryInterface(tabNode));
  rel.AppendTarget(tabContent);
  return rel;
}
