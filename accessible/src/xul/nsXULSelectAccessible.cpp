






































#include "nsXULSelectAccessible.h"
#include "nsAccessibilityService.h"
#include "nsIContent.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULTextboxElement.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsCaseTreatment.h"





















nsXULListboxAccessible::nsXULListboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsXULSelectableAccessible(aDOMNode, aShell)
{
}







NS_IMETHODIMP
nsXULListboxAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  nsresult rv = nsAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);


  nsCOMPtr<nsIDOMElement> element (do_QueryInterface(mDOMNode));
  if (element) {
    nsAutoString selType;
    element->GetAttribute(NS_LITERAL_STRING("seltype"), selType);
    if (!selType.IsEmpty() && selType.EqualsLiteral("multiple"))
      *aState |= nsIAccessibleStates::STATE_MULTISELECTABLE |
                 nsIAccessibleStates::STATE_EXTSELECTABLE;
  }

  return NS_OK;
}




NS_IMETHODIMP nsXULListboxAccessible::GetValue(nsAString& _retval)
{
  _retval.Truncate();
  nsCOMPtr<nsIDOMXULSelectControlElement> select(do_QueryInterface(mDOMNode));
  if (select) {
    nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
    select->GetSelectedItem(getter_AddRefs(selectedItem));
    if (selectedItem)
      return selectedItem->GetLabel(_retval);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULListboxAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_LIST;
  return NS_OK;
}




nsXULListitemAccessible::nsXULListitemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsXULMenuitemAccessible(aDOMNode, aShell)
{
  mIsCheckbox = PR_FALSE;
  nsCOMPtr<nsIDOMElement> listItem (do_QueryInterface(mDOMNode));
  if (listItem) {
    nsAutoString typeString;
    nsresult res = listItem->GetAttribute(NS_LITERAL_STRING("type"), typeString);
    if (NS_SUCCEEDED(res) && typeString.Equals(NS_LITERAL_STRING("checkbox")))
      mIsCheckbox = PR_TRUE;
  }
}


NS_IMPL_ISUPPORTS_INHERITED0(nsXULListitemAccessible, nsAccessible)





NS_IMETHODIMP nsXULListitemAccessible::GetName(nsAString& _retval)
{
  nsCOMPtr<nsIDOMNode> child;
  if (NS_SUCCEEDED(mDOMNode->GetFirstChild(getter_AddRefs(child)))) {
    nsCOMPtr<nsIDOMElement> childElement (do_QueryInterface(child));
    if (childElement) {
      nsAutoString tagName;
      childElement->GetLocalName(tagName);
      if (tagName.EqualsLiteral("listcell")) {
        childElement->GetAttribute(NS_LITERAL_STRING("label"), _retval);
        return NS_OK;
      }
    }
  }
  return GetXULName(_retval);
}




NS_IMETHODIMP nsXULListitemAccessible::GetRole(PRUint32 *aRole)
{
  if (mIsCheckbox)
    *aRole = nsIAccessibleRole::ROLE_CHECKBUTTON;
  else
    *aRole = nsIAccessibleRole::ROLE_LISTITEM;
  return NS_OK;
}




NS_IMETHODIMP
nsXULListitemAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  if (mIsCheckbox) {
    nsXULMenuitemAccessible::GetState(aState, aExtraState);
    return NS_OK;
  }

  *aState = nsIAccessibleStates::STATE_FOCUSABLE |
            nsIAccessibleStates::STATE_SELECTABLE;
  nsCOMPtr<nsIDOMXULSelectControlItemElement> listItem (do_QueryInterface(mDOMNode));
  if (listItem) {
    PRBool isSelected;
    listItem->GetSelected(&isSelected);
    if (isSelected)
      *aState |= nsIAccessibleStates::STATE_SELECTED;

    if (gLastFocusedNode == mDOMNode) {
      *aState |= nsIAccessibleStates::STATE_FOCUSED;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULListitemAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click && mIsCheckbox) {
    
    PRUint32 state;
    GetState(&state, nsnull);

    if (state & nsIAccessibleStates::STATE_CHECKED)
      aName.AssignLiteral("uncheck");
    else
      aName.AssignLiteral("check");

    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}








nsXULComboboxAccessible::nsXULComboboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsAccessibleWrap(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsXULComboboxAccessible::Init()
{
  nsresult rv = nsAccessibleWrap::Init();
  nsXULMenupopupAccessible::GenerateMenu(mDOMNode);
  return rv;
}


NS_IMETHODIMP nsXULComboboxAccessible::GetRole(PRUint32 *aRole)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
  if (!content) {
    return NS_ERROR_FAILURE;
  }
  *aRole = content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                                NS_LITERAL_STRING("autocomplete"), eIgnoreCase) ?
                                nsIAccessibleRole::ROLE_AUTOCOMPLETE :
                                nsIAccessibleRole::ROLE_COMBOBOX;
  return NS_OK;
}










NS_IMETHODIMP
nsXULComboboxAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  nsresult rv = nsAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mDOMNode));
  if (menuList) {
    PRBool isOpen;
    menuList->GetOpen(&isOpen);
    if (isOpen) {
      *aState |= nsIAccessibleStates::STATE_EXPANDED;
    }
    else {
      *aState |= nsIAccessibleStates::STATE_COLLAPSED;
    }
    PRBool isEditable;
    menuList->GetEditable(&isEditable);
    if (!isEditable) {
      *aState |= nsIAccessibleStates::STATE_READONLY;
    }
  }

  *aState |= nsIAccessibleStates::STATE_HASPOPUP |
             nsIAccessibleStates::STATE_FOCUSABLE;

  return NS_OK;
}

NS_IMETHODIMP nsXULComboboxAccessible::GetValue(nsAString& _retval)
{
  _retval.Truncate();

  
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mDOMNode));
  if (menuList) {
    return menuList->GetLabel(_retval);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULComboboxAccessible::GetDescription(nsAString& aDescription)
{
  
  aDescription.Truncate();
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mDOMNode));
  if (!menuList) {
    return NS_ERROR_FAILURE;  
  }
  nsCOMPtr<nsIDOMXULSelectControlItemElement> focusedOption;
  menuList->GetSelectedItem(getter_AddRefs(focusedOption));
  nsCOMPtr<nsIDOMNode> focusedOptionNode(do_QueryInterface(focusedOption));
  if (focusedOptionNode) {
    nsCOMPtr<nsIAccessibilityService> accService = 
      do_GetService("@mozilla.org/accessibilityService;1");
    NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);
    nsCOMPtr<nsIAccessible> focusedOptionAccessible;
    accService->GetAccessibleInWeakShell(focusedOptionNode, mWeakShell, 
                                        getter_AddRefs(focusedOptionAccessible));
    NS_ENSURE_TRUE(focusedOptionAccessible, NS_ERROR_FAILURE);
    return focusedOptionAccessible->GetDescription(aDescription);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXULComboboxAccessible::GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);

  if (content->NodeInfo()->Equals(nsAccessibilityAtoms::textbox, kNameSpaceID_XUL) ||
      content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::editable,
                           nsAccessibilityAtoms::_true, eIgnoreCase)) {
    
    
    
    *aAllowsAnonChildren = PR_TRUE;
  } else {
    
    
    *aAllowsAnonChildren = PR_FALSE;
  }
  return NS_OK;
}


NS_IMETHODIMP nsXULComboboxAccessible::GetNumActions(PRUint8 *aNumActions)
{
  *aNumActions = 1;
  return NS_OK;
}




NS_IMETHODIMP nsXULComboboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != nsXULComboboxAccessible::eAction_Click) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mDOMNode));
  if (!menuList) {
    return NS_ERROR_FAILURE;
  }
  PRBool isDroppedDown;
  menuList->GetOpen(&isDroppedDown);
  return menuList->SetOpen(!isDroppedDown);
}







NS_IMETHODIMP nsXULComboboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != nsXULComboboxAccessible::eAction_Click) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mDOMNode));
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

