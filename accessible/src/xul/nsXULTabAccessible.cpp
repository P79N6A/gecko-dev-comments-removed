






































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


nsresult
nsXULTabAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PAGETAB;
  return NS_OK;
}




nsresult
nsXULTabAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  nsresult rv = nsLeafAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  
  
  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
  if (presShell && content) {
    nsIFrame *frame = presShell->GetPrimaryFrameFor(content);
    if (frame) {
      const nsStyleUserInterface* ui = frame->GetStyleUserInterface();
      if (ui->mUserFocus == NS_STYLE_USER_FOCUS_NORMAL)
        *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
    }
  }
  
  *aState |= nsIAccessibleStates::STATE_SELECTABLE;
  *aState &= ~nsIAccessibleStates::STATE_SELECTED;
  nsCOMPtr<nsIDOMXULSelectControlItemElement> tab(do_QueryInterface(mDOMNode));
  if (tab) {
    PRBool selected = PR_FALSE;
    if (NS_SUCCEEDED(tab->GetSelected(&selected)) && selected)
      *aState |= nsIAccessibleStates::STATE_SELECTED;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXULTabAccessible::GetRelationByType(PRUint32 aRelationType,
                                      nsIAccessibleRelation **aRelation)
{
  nsresult rv = nsLeafAccessible::GetRelationByType(aRelationType,
                                                    aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRelationType != nsIAccessibleRelation::RELATION_LABEL_FOR)
    return NS_OK;

  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  
  
  rv = nsRelUtils::AddTargetFromIDRefAttr(aRelationType, aRelation, content,
                                          nsAccessibilityAtoms::linkedPanel,
                                          PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rv != NS_OK_NO_RELATION_TARGET)
    return NS_OK;

  
  
  

  nsCOMPtr<nsIAccessible> tabsAcc = GetParent();
  NS_ENSURE_TRUE(nsAccUtils::Role(tabsAcc) == nsIAccessibleRole::ROLE_PAGETABLIST,
                 NS_ERROR_FAILURE);

  PRInt32 tabIndex = -1;

  nsCOMPtr<nsIAccessible> childAcc;
  tabsAcc->GetFirstChild(getter_AddRefs(childAcc));
  while (childAcc) {
    if (nsAccUtils::Role(childAcc) == nsIAccessibleRole::ROLE_PAGETAB)
      tabIndex++;

    if (childAcc == this)
      break;

    nsCOMPtr<nsIAccessible> acc;
    childAcc->GetNextSibling(getter_AddRefs(acc));
    childAcc.swap(acc);
  }

  nsCOMPtr<nsIAccessible> tabBoxAcc;
  tabsAcc->GetParent(getter_AddRefs(tabBoxAcc));
  NS_ENSURE_TRUE(nsAccUtils::Role(tabBoxAcc) == nsIAccessibleRole::ROLE_PANE,
                 NS_ERROR_FAILURE);

  tabBoxAcc->GetFirstChild(getter_AddRefs(childAcc));
  while (childAcc) {
    if (nsAccUtils::Role(childAcc) == nsIAccessibleRole::ROLE_PROPERTYPAGE) {
      if (tabIndex == 0)
        return nsRelUtils::AddTarget(aRelationType, aRelation, childAcc);

      tabIndex--;
    }

    nsCOMPtr<nsIAccessible> acc;
    childAcc->GetNextSibling(getter_AddRefs(acc));
    childAcc.swap(acc);
  }

  return NS_OK;
}

nsresult
nsXULTabAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  NS_ENSURE_TRUE(mDOMNode, NS_ERROR_FAILURE);

  nsresult rv = nsLeafAccessible::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAccUtils::SetAccAttrsForXULSelectControlItem(mDOMNode, aAttributes);

  return NS_OK;
}








nsXULTabBoxAccessible::nsXULTabBoxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{ 
}


nsresult
nsXULTabBoxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PANE;
  return NS_OK;
}

#ifdef NEVER

NS_IMETHODIMP nsXULTabBoxAccessible::GetChildCount(PRInt32 *_retval)
{
  *_retval = 2;
  return NS_OK;
}
#endif






nsXULTabsAccessible::nsXULTabsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsXULSelectableAccessible(aNode, aShell)
{ 
}


nsresult
nsXULTabsAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PAGETABLIST;
  return NS_OK;
}


NS_IMETHODIMP nsXULTabsAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 0;
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




nsXULTabpanelAccessible::
  nsXULTabpanelAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
  nsAccessibleWrap(aNode, aShell)
{
}

nsresult
nsXULTabpanelAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PROPERTYPAGE;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTabpanelAccessible::GetRelationByType(PRUint32 aRelationType,
                                           nsIAccessibleRelation **aRelation)
{
  nsresult rv = nsAccessibleWrap::GetRelationByType(aRelationType, aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRelationType != nsIAccessibleRelation::RELATION_LABELLED_BY)
    return NS_OK;

  
  nsCOMPtr<nsIAccessible> tabBoxAcc;
  GetParent(getter_AddRefs(tabBoxAcc));
  NS_ENSURE_TRUE(nsAccUtils::Role(tabBoxAcc) == nsIAccessibleRole::ROLE_PANE,
                 NS_ERROR_FAILURE);

  PRInt32 tabpanelIndex = -1;
  nsCOMPtr<nsIAccessible> tabsAcc;

  PRBool isTabpanelFound = PR_FALSE;
  nsCOMPtr<nsIAccessible> childAcc;
  tabBoxAcc->GetFirstChild(getter_AddRefs(childAcc));
  while (childAcc && (!tabsAcc || !isTabpanelFound)) {
    if (nsAccUtils::Role(childAcc) == nsIAccessibleRole::ROLE_PAGETABLIST)
      tabsAcc = childAcc;

    if (!isTabpanelFound &&
        nsAccUtils::Role(childAcc) == nsIAccessibleRole::ROLE_PROPERTYPAGE)
      tabpanelIndex++;

    if (childAcc == this)
      isTabpanelFound = PR_TRUE;

    nsCOMPtr<nsIAccessible> acc;
    childAcc->GetNextSibling(getter_AddRefs(acc));
    childAcc.swap(acc);
  }

  if (!tabsAcc || tabpanelIndex == -1)
    return NS_OK;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  nsIAtom *atomID = content->GetID();

  nsCOMPtr<nsIAccessible> foundTabAcc;
  tabsAcc->GetFirstChild(getter_AddRefs(childAcc));
  while (childAcc) {
    if (nsAccUtils::Role(childAcc) == nsIAccessibleRole::ROLE_PAGETAB) {
      if (atomID) {
        nsCOMPtr<nsIAccessNode> tabAccNode(do_QueryInterface(childAcc));
        nsCOMPtr<nsIDOMNode> tabNode;
        tabAccNode->GetDOMNode(getter_AddRefs(tabNode));
        nsCOMPtr<nsIContent> tabContent(do_QueryInterface(tabNode));
        NS_ENSURE_TRUE(tabContent, NS_ERROR_FAILURE);

        if (tabContent->AttrValueIs(kNameSpaceID_None,
                                    nsAccessibilityAtoms::linkedPanel, atomID,
                                    eCaseMatters)) {
          return nsRelUtils::AddTarget(aRelationType, aRelation, childAcc);
        }
      }

      if (tabpanelIndex == 0) {
        foundTabAcc = childAcc;
        if (!atomID)
          break;
      }

      tabpanelIndex--;
    }

    nsCOMPtr<nsIAccessible> acc;
    childAcc->GetNextSibling(getter_AddRefs(acc));
    childAcc.swap(acc);
  }

  return nsRelUtils::AddTarget(aRelationType, aRelation, foundTabAcc);
}

