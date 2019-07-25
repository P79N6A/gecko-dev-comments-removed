







































#include "nsXULComboboxAccessible.h"

#include "nsAccessibilityService.h"
#include "nsCoreUtils.h"

#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"





nsXULComboboxAccessible::
  nsXULComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

nsresult
nsXULComboboxAccessible::Init()
{
  nsresult rv = nsAccessibleWrap::Init();
  nsCoreUtils::GeneratePopupTree(mContent);
  return rv;
}

nsresult
nsXULComboboxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::autocomplete, eIgnoreCase)) {
    *aRole = nsIAccessibleRole::ROLE_AUTOCOMPLETE;
  } else {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX;
  }

  return NS_OK;
}

nsresult
nsXULComboboxAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  
  
  
  
  
  

  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList) {
    PRBool isOpen;
    menuList->GetOpen(&isOpen);
    if (isOpen) {
      *aState |= nsIAccessibleStates::STATE_EXPANDED;
    }
    else {
      *aState |= nsIAccessibleStates::STATE_COLLAPSED;
    }
  }

  *aState |= nsIAccessibleStates::STATE_HASPOPUP |
             nsIAccessibleStates::STATE_FOCUSABLE;

  return NS_OK;
}

NS_IMETHODIMP
nsXULComboboxAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList)
    return menuList->GetLabel(aValue);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULComboboxAccessible::GetDescription(nsAString& aDescription)
{
  aDescription.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMXULMenuListElement> menuListElm(do_QueryInterface(mContent));
  if (!menuListElm)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> focusedOptionItem;
  menuListElm->GetSelectedItem(getter_AddRefs(focusedOptionItem));
  nsCOMPtr<nsIContent> focusedOptionContent =
    do_QueryInterface(focusedOptionItem);
  if (focusedOptionContent) {
    nsAccessible *focusedOption =
      GetAccService()->GetAccessibleInWeakShell(focusedOptionContent, mWeakShell);
    NS_ENSURE_TRUE(focusedOption, NS_ERROR_FAILURE);

    return focusedOption->GetDescription(aDescription);
  }

  return NS_OK;
}

PRBool
nsXULComboboxAccessible::GetAllowsAnonChildAccessibles()
{
  if (mContent->NodeInfo()->Equals(nsAccessibilityAtoms::textbox, kNameSpaceID_XUL) ||
      mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::editable,
                            nsAccessibilityAtoms::_true, eIgnoreCase)) {
    
    
    
    return PR_TRUE;
  }

  
  
  return PR_FALSE;
}

NS_IMETHODIMP
nsXULComboboxAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);

  
  *aNumActions = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXULComboboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != nsXULComboboxAccessible::eAction_Click) {
    return NS_ERROR_INVALID_ARG;
  }

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (!menuList) {
    return NS_ERROR_FAILURE;
  }
  PRBool isDroppedDown;
  menuList->GetOpen(&isDroppedDown);
  return menuList->SetOpen(!isDroppedDown);
}

NS_IMETHODIMP
nsXULComboboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != nsXULComboboxAccessible::eAction_Click) {
    return NS_ERROR_INVALID_ARG;
  }

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  
  
  

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (!menuList) {
    return NS_ERROR_FAILURE;
  }
  PRBool isDroppedDown;
  menuList->GetOpen(&isDroppedDown);
  if (isDroppedDown)
    aName.AssignLiteral("close"); 
  else
    aName.AssignLiteral("open"); 

  return NS_OK;
}
