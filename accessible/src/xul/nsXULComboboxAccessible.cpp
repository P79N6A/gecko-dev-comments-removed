







































#include "nsXULComboboxAccessible.h"

#include "States.h"
#include "nsAccessibilityService.h"
#include "nsCoreUtils.h"

#include "nsIAutoCompleteInput.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"

using namespace mozilla::a11y;





nsXULComboboxAccessible::
  nsXULComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::autocomplete, eIgnoreCase))
    mFlags |= eAutoCompleteAccessible;
  else
    mFlags |= eComboboxAccessible;
}

PRUint32
nsXULComboboxAccessible::NativeRole()
{
  if (IsAutoComplete())
    return nsIAccessibleRole::ROLE_AUTOCOMPLETE;
  return nsIAccessibleRole::ROLE_COMBOBOX;
}

PRUint64
nsXULComboboxAccessible::NativeState()
{
  
  
  
  
  
  

  
  PRUint64 states = nsAccessible::NativeState();

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList) {
    PRBool isOpen;
    menuList->GetOpen(&isOpen);
    if (isOpen) {
      states |= states::EXPANDED;
    }
    else {
      states |= states::COLLAPSED;
    }
  }

  states |= states::HASPOPUP | states::FOCUSABLE;

  return states;
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

void
nsXULComboboxAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();
  
  nsCOMPtr<nsIDOMXULMenuListElement> menuListElm(do_QueryInterface(mContent));
  if (!menuListElm)
    return;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> focusedOptionItem;
  menuListElm->GetSelectedItem(getter_AddRefs(focusedOptionItem));
  nsCOMPtr<nsIContent> focusedOptionContent =
    do_QueryInterface(focusedOptionItem);
  if (focusedOptionContent) {
    nsAccessible* focusedOptionAcc = GetAccService()->
      GetAccessibleInWeakShell(focusedOptionContent, mWeakShell);
    if (focusedOptionAcc)
      focusedOptionAcc->Description(aDescription);
  }
}

PRBool
nsXULComboboxAccessible::GetAllowsAnonChildAccessibles()
{
  if (mContent->NodeInfo()->Equals(nsGkAtoms::textbox, kNameSpaceID_XUL) ||
      mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::editable,
                            nsGkAtoms::_true, eIgnoreCase)) {
    
    
    
    return PR_TRUE;
  }

  
  
  return PR_FALSE;
}
PRUint8
nsXULComboboxAccessible::ActionCount()
{
  
  return 1;
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




bool
nsXULComboboxAccessible::IsActiveWidget() const
{
  if (IsAutoComplete() ||
     mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::editable,
                           nsGkAtoms::_true, eIgnoreCase)) {
    PRInt32 childCount = mChildren.Length();
    for (PRInt32 idx = 0; idx < childCount; idx++) {
      nsAccessible* child = mChildren[idx];
      if (child->Role() == nsIAccessibleRole::ROLE_ENTRY)
        return FocusMgr()->HasDOMFocus(child->GetContent());
    }
    return false;
  }

  return FocusMgr()->HasDOMFocus(mContent);
}

bool
nsXULComboboxAccessible::AreItemsOperable() const
{
  if (IsAutoComplete()) {
    nsCOMPtr<nsIAutoCompleteInput> autoCompleteInputElm =
      do_QueryInterface(mContent);
    if (autoCompleteInputElm) {
      PRBool isOpen = PR_FALSE;
      autoCompleteInputElm->GetPopupOpen(&isOpen);
      return isOpen;
    }
    return false;
  }

  nsCOMPtr<nsIDOMXULMenuListElement> menuListElm = do_QueryInterface(mContent);
  if (menuListElm) {
    PRBool isOpen = PR_FALSE;
    menuListElm->GetOpen(&isOpen);
    return isOpen;
  }

  return false;
}
