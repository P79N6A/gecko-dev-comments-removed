





































#include "nsXULMenuAccessible.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIMutableArray.h"
#include "nsIDOMXULContainerElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMKeyEvent.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsGUIEvent.h"
#include "nsXULFormControlAccessible.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"


static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);






nsXULSelectableAccessible::nsXULSelectableAccessible(nsIDOMNode* aDOMNode,
                                                     nsIWeakReference* aShell):
nsAccessibleWrap(aDOMNode, aShell)
{
  mSelectControl = do_QueryInterface(aDOMNode);
}

NS_IMPL_ISUPPORTS_INHERITED1(nsXULSelectableAccessible, nsAccessible, nsIAccessibleSelectable)

nsresult
nsXULSelectableAccessible::Shutdown()
{
  mSelectControl = nsnull;
  return nsAccessibleWrap::Shutdown();
}

nsresult nsXULSelectableAccessible::ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState)
{
  *aSelState = PR_FALSE;

  if (!mSelectControl) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIAccessible> childAcc;
  GetChildAt(aIndex, getter_AddRefs(childAcc));
  nsCOMPtr<nsIAccessNode> accNode = do_QueryInterface(childAcc);
  NS_ENSURE_TRUE(accNode, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> childNode;
  accNode->GetDOMNode(getter_AddRefs(childNode));
  nsCOMPtr<nsIDOMXULSelectControlItemElement> item(do_QueryInterface(childNode));
  NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);

  item->GetSelected(aSelState);
  if (eSelection_GetState == aMethod) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);

  if (eSelection_Add == aMethod && !(*aSelState)) {
    return xulMultiSelect ? xulMultiSelect->AddItemToSelection(item) :
                            mSelectControl->SetSelectedItem(item);
  }
  if (eSelection_Remove == aMethod && (*aSelState)) {
    return xulMultiSelect ? xulMultiSelect->RemoveItemFromSelection(item) :
                            mSelectControl->SetSelectedItem(nsnull);
  }
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsXULSelectableAccessible::GetSelectedChildren(nsIArray **aChildren)
{
  *aChildren = nsnull;
  if (!mSelectControl) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessibilityService> accService = GetAccService();
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);

  nsCOMPtr<nsIMutableArray> selectedAccessibles =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_STATE(selectedAccessibles);

  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);
  nsCOMPtr<nsIAccessible> selectedAccessible;
  if (xulMultiSelect) {
    PRInt32 length = 0;
    xulMultiSelect->GetSelectedCount(&length);
    for (PRInt32 index = 0; index < length; index++) {
      nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
      xulMultiSelect->GetSelectedItem(index, getter_AddRefs(selectedItem));
      nsCOMPtr<nsIDOMNode> selectedNode(do_QueryInterface(selectedItem));
      accService->GetAccessibleInWeakShell(selectedNode, mWeakShell,
                                           getter_AddRefs(selectedAccessible));
      if (selectedAccessible)
        selectedAccessibles->AppendElement(selectedAccessible, PR_FALSE);
    }
  }
  else {  
    nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
    mSelectControl->GetSelectedItem(getter_AddRefs(selectedItem));
    nsCOMPtr<nsIDOMNode> selectedNode(do_QueryInterface(selectedItem));
    if(selectedNode) {
      accService->GetAccessibleInWeakShell(selectedNode, mWeakShell,
                                           getter_AddRefs(selectedAccessible));
      if (selectedAccessible)
        selectedAccessibles->AppendElement(selectedAccessible, PR_FALSE);
    }
  }

  PRUint32 uLength = 0;
  selectedAccessibles->GetLength(&uLength);
  if (uLength != 0) { 
    NS_ADDREF(*aChildren = selectedAccessibles);
  }

  return NS_OK;
}


NS_IMETHODIMP nsXULSelectableAccessible::RefSelection(PRInt32 aIndex, nsIAccessible **aAccessible)
{
  *aAccessible = nsnull;
  if (!mSelectControl) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);
  if (xulMultiSelect)
    xulMultiSelect->GetSelectedItem(aIndex, getter_AddRefs(selectedItem));

  if (aIndex == 0)
    mSelectControl->GetSelectedItem(getter_AddRefs(selectedItem));

  if (selectedItem) {
    nsCOMPtr<nsIAccessibilityService> accService = GetAccService();
    if (accService) {
      accService->GetAccessibleInWeakShell(selectedItem, mWeakShell, aAccessible);
      if (*aAccessible) {
        NS_ADDREF(*aAccessible);
        return NS_OK;
      }
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULSelectableAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  *aSelectionCount = 0;
  if (!mSelectControl) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);
  if (xulMultiSelect)
    return xulMultiSelect->GetSelectedCount(aSelectionCount);

  
  PRInt32 index;
  mSelectControl->GetSelectedIndex(&index);
  if (index >= 0)
    *aSelectionCount = 1;
  return NS_OK;
}

NS_IMETHODIMP nsXULSelectableAccessible::AddChildToSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Add, &isSelected);
}

NS_IMETHODIMP nsXULSelectableAccessible::RemoveChildFromSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Remove, &isSelected);
}

NS_IMETHODIMP nsXULSelectableAccessible::IsChildSelected(PRInt32 aIndex, PRBool *aIsSelected)
{
  *aIsSelected = PR_FALSE;
  return ChangeSelection(aIndex, eSelection_GetState, aIsSelected);
}

NS_IMETHODIMP nsXULSelectableAccessible::ClearSelection()
{
  if (!mSelectControl) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);
  return xulMultiSelect ? xulMultiSelect->ClearSelection() : mSelectControl->SetSelectedIndex(-1);
}

NS_IMETHODIMP nsXULSelectableAccessible::SelectAllSelection(PRBool *aSucceeded)
{
  *aSucceeded = PR_TRUE;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);
  if (xulMultiSelect)
    return xulMultiSelect->SelectAll();

  
  *aSucceeded = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}




nsXULMenuitemAccessible::nsXULMenuitemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell): 
nsAccessibleWrap(aDOMNode, aShell)
{ 
}

nsresult
nsXULMenuitemAccessible::Init()
{
  nsresult rv = nsAccessibleWrap::Init();
  nsCoreUtils::GeneratePopupTree(mDOMNode);
  return rv;
}

nsresult
nsXULMenuitemAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  if (!element)
    return NS_ERROR_FAILURE;
  PRBool isFocused = PR_FALSE;
  element->HasAttribute(NS_LITERAL_STRING("_moz-menuactive"), &isFocused); 
  if (isFocused)
    *aState |= nsIAccessibleStates::STATE_FOCUSED;

  
  nsAutoString tagName;
  element->GetLocalName(tagName);
  if (tagName.EqualsLiteral("menu")) {
    *aState |= nsIAccessibleStates::STATE_HASPOPUP;
    PRBool isOpen;
    element->HasAttribute(NS_LITERAL_STRING("open"), &isOpen);
    if (isOpen) {
      *aState |= nsIAccessibleStates::STATE_EXPANDED;
    }
    else {
      *aState |= nsIAccessibleStates::STATE_COLLAPSED;
    }
  }

  nsAutoString menuItemType;
  element->GetAttribute(NS_LITERAL_STRING("type"), menuItemType); 

  if (!menuItemType.IsEmpty()) {
    
    if (menuItemType.EqualsIgnoreCase("radio") ||
        menuItemType.EqualsIgnoreCase("checkbox"))
      *aState |= nsIAccessibleStates::STATE_CHECKABLE;

    
    nsAutoString checkValue;
    element->GetAttribute(NS_LITERAL_STRING("checked"), checkValue);
    if (checkValue.EqualsLiteral("true")) {
      *aState |= nsIAccessibleStates::STATE_CHECKED;
    }
  }

  
  PRBool isComboboxOption =
    (nsAccUtils::Role(this) == nsIAccessibleRole::ROLE_COMBOBOX_OPTION);
  if (isComboboxOption) {
    
    PRBool isSelected = PR_FALSE;
    nsCOMPtr<nsIDOMXULSelectControlItemElement>
      item(do_QueryInterface(mDOMNode));
    NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);
    item->GetSelected(&isSelected);

    
    PRBool isCollapsed = PR_FALSE;
    nsCOMPtr<nsIAccessible> parentAccessible(GetParent());
    if (nsAccUtils::State(parentAccessible) & nsIAccessibleStates::STATE_INVISIBLE)
      isCollapsed = PR_TRUE;
    
    if (isSelected) {
      *aState |= nsIAccessibleStates::STATE_SELECTED;
      
      
      if (isCollapsed) {
        
        nsCOMPtr<nsIAccessible> grandParentAcc;
        parentAccessible->GetParent(getter_AddRefs(grandParentAcc));
        NS_ENSURE_TRUE(grandParentAcc, NS_ERROR_FAILURE);
        NS_ASSERTION(nsAccUtils::Role(grandParentAcc) == nsIAccessibleRole::ROLE_COMBOBOX,
                     "grandparent of combobox listitem is not combobox");
        PRUint32 grandParentState, grandParentExtState;
        grandParentAcc->GetState(&grandParentState, &grandParentExtState);
        *aState &= ~(nsIAccessibleStates::STATE_OFFSCREEN |
                     nsIAccessibleStates::STATE_INVISIBLE);
        *aState |= grandParentState & nsIAccessibleStates::STATE_OFFSCREEN |
                   grandParentState & nsIAccessibleStates::STATE_INVISIBLE;
        if (aExtraState) {
          *aExtraState |=
            grandParentExtState & nsIAccessibleStates::EXT_STATE_OPAQUE;
        }
      } 
    } 
  } 

  
  
  if (*aState & nsIAccessibleStates::STATE_UNAVAILABLE) {
    
    nsCOMPtr<nsILookAndFeel> lookNFeel(do_GetService(kLookAndFeelCID));
    PRInt32 skipDisabledMenuItems = 0;
    lookNFeel->GetMetric(nsILookAndFeel::eMetric_SkipNavigatingDisabledMenuItem,
                         skipDisabledMenuItems);
    
    
    if (skipDisabledMenuItems || isComboboxOption) {
      return NS_OK;
    }
  }
  *aState|= (nsIAccessibleStates::STATE_FOCUSABLE |
             nsIAccessibleStates::STATE_SELECTABLE);

  return NS_OK;
}

nsresult
nsXULMenuitemAccessible::GetNameInternal(nsAString& aName)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::label, aName);
  return NS_OK;
}

NS_IMETHODIMP nsXULMenuitemAccessible::GetDescription(nsAString& aDescription)
{
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  if (!element) {
    return NS_ERROR_FAILURE;
  }
  element->GetAttribute(NS_LITERAL_STRING("description"), aDescription);

  return NS_OK;
}


NS_IMETHODIMP
nsXULMenuitemAccessible::GetKeyboardShortcut(nsAString& aAccessKey)
{
  aAccessKey.Truncate();

  static PRInt32 gMenuAccesskeyModifier = -1;  

  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(mDOMNode));
  if (elt) {
    nsAutoString accesskey;
    
    
    elt->GetAttribute(NS_LITERAL_STRING("accesskey"), accesskey);
    if (accesskey.IsEmpty())
      return NS_OK;

    nsCOMPtr<nsIAccessible> parentAccessible(GetParent());
    if (parentAccessible) {
      if (nsAccUtils::RoleInternal(parentAccessible) ==
          nsIAccessibleRole::ROLE_MENUBAR) {
        
        
        if (gMenuAccesskeyModifier == -1) {
          
          gMenuAccesskeyModifier = 0;
          nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
          if (prefBranch)
            prefBranch->GetIntPref("ui.key.menuAccessKey", &gMenuAccesskeyModifier);
        }
        nsAutoString propertyKey;
        switch (gMenuAccesskeyModifier) {
          case nsIDOMKeyEvent::DOM_VK_CONTROL: propertyKey.AssignLiteral("VK_CONTROL"); break;
          case nsIDOMKeyEvent::DOM_VK_ALT: propertyKey.AssignLiteral("VK_ALT"); break;
          case nsIDOMKeyEvent::DOM_VK_META: propertyKey.AssignLiteral("VK_META"); break;
        }
        if (!propertyKey.IsEmpty())
          nsAccessible::GetFullKeyName(propertyKey, accesskey, aAccessKey);
      }
    }
    if (aAccessKey.IsEmpty())
      aAccessKey = accesskey;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsXULMenuitemAccessible::GetDefaultKeyBinding(nsAString& aKeyBinding)
{
  aKeyBinding.Truncate();

  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(mDOMNode));
  NS_ENSURE_TRUE(elt, NS_ERROR_FAILURE);

  nsAutoString accelText;
  elt->GetAttribute(NS_LITERAL_STRING("acceltext"), accelText);
  if (accelText.IsEmpty())
    return NS_OK;

  aKeyBinding = accelText;

  return NS_OK;
}

nsresult
nsXULMenuitemAccessible::GetRoleInternal(PRUint32 *aRole)
{
  nsCOMPtr<nsIDOMXULContainerElement> xulContainer(do_QueryInterface(mDOMNode));
  if (xulContainer) {
    *aRole = nsIAccessibleRole::ROLE_PARENT_MENUITEM;
    return NS_OK;
  }

  nsCOMPtr<nsIAccessible> parent;
  GetParent(getter_AddRefs(parent));
  if (nsAccUtils::Role(parent) == nsIAccessibleRole::ROLE_COMBOBOX_LIST) {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_OPTION;
    return NS_OK;
  }

  *aRole = nsIAccessibleRole::ROLE_MENUITEM;
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  if (!element)
    return NS_ERROR_FAILURE;
  nsAutoString menuItemType;
  element->GetAttribute(NS_LITERAL_STRING("type"), menuItemType);
  if (menuItemType.EqualsIgnoreCase("radio"))
    *aRole = nsIAccessibleRole::ROLE_RADIO_MENU_ITEM;
  else if (menuItemType.EqualsIgnoreCase("checkbox"))
    *aRole = nsIAccessibleRole::ROLE_CHECK_MENU_ITEM;

  return NS_OK;
}

nsresult
nsXULMenuitemAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  NS_ENSURE_TRUE(mDOMNode, NS_ERROR_FAILURE);

  nsresult rv = nsAccessible::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAccUtils::SetAccAttrsForXULContainerItem(mDOMNode, aAttributes);
  return NS_OK;
}

PRBool
nsXULMenuitemAccessible::GetAllowsAnonChildAccessibles()
{
  
  return PR_FALSE;
}

NS_IMETHODIMP nsXULMenuitemAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click) {   
    DoCommand();
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsXULMenuitemAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("click"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsXULMenuitemAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}




nsXULMenuSeparatorAccessible::nsXULMenuSeparatorAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell): 
nsXULMenuitemAccessible(aDOMNode, aShell)
{ 
}

nsresult
nsXULMenuSeparatorAccessible::GetStateInternal(PRUint32 *aState,
                                               PRUint32 *aExtraState)
{
  
  nsresult rv = nsXULMenuitemAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState &= (nsIAccessibleStates::STATE_OFFSCREEN | 
              nsIAccessibleStates::STATE_INVISIBLE);

  return NS_OK;
}

nsresult
nsXULMenuSeparatorAccessible::GetNameInternal(nsAString& aName)
{
  return NS_OK;
}

nsresult
nsXULMenuSeparatorAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_SEPARATOR;
  return NS_OK;
}

NS_IMETHODIMP nsXULMenuSeparatorAccessible::DoAction(PRUint8 index)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULMenuSeparatorAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULMenuSeparatorAccessible::GetNumActions(PRUint8 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsXULMenupopupAccessible::nsXULMenupopupAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell): 
  nsXULSelectableAccessible(aDOMNode, aShell)
{ 
  
  nsCOMPtr<nsIDOMNode> parentNode;
  aDOMNode->GetParentNode(getter_AddRefs(parentNode));
  mSelectControl = do_QueryInterface(parentNode);
}

nsresult
nsXULMenupopupAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

#ifdef DEBUG_A11Y
  
  PRBool isActive = PR_FALSE;

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  element->HasAttribute(NS_LITERAL_STRING("menuactive"), &isActive);
  if (!isActive) {
    nsCOMPtr<nsIAccessible> parent(GetParent());
    nsCOMPtr<nsIDOMNode> parentNode;
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(parent));
    if (accessNode) 
      accessNode->GetDOMNode(getter_AddRefs(parentNode));
    element = do_QueryInterface(parentNode);
    if (element)
      element->HasAttribute(NS_LITERAL_STRING("open"), &isActive);
  }
  NS_ASSERTION(isActive || *aState & nsIAccessibleStates::STATE_INVISIBLE,
               "XULMenupopup doesn't have STATE_INVISIBLE when it's inactive");
#endif

  if (*aState & nsIAccessibleStates::STATE_INVISIBLE)
    *aState |= (nsIAccessibleStates::STATE_OFFSCREEN |
                nsIAccessibleStates::STATE_COLLAPSED);

  return NS_OK;
}

nsresult
nsXULMenupopupAccessible::GetNameInternal(nsAString& aName)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  while (content && aName.IsEmpty()) {
    content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::label, aName);
    content = content->GetParent();
  }

  return NS_OK;
}

nsresult
nsXULMenupopupAccessible::GetRoleInternal(PRUint32 *aRole)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIAccessible> parent;
  GetParent(getter_AddRefs(parent));
  if (parent) {
    PRUint32 role = nsAccUtils::Role(parent);
    if (role == nsIAccessibleRole::ROLE_COMBOBOX ||
        role == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
      *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
      return NS_OK;

    } else if (role == nsIAccessibleRole::ROLE_PUSHBUTTON) {
      
      nsCOMPtr<nsIAccessible> grandParent;
      parent->GetParent(getter_AddRefs(grandParent));
      if (role == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
        *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
        return NS_OK;
      }
    }
  }

  *aRole = nsIAccessibleRole::ROLE_MENUPOPUP;
  return NS_OK;
}



nsXULMenubarAccessible::nsXULMenubarAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell): 
  nsAccessibleWrap(aDOMNode, aShell)
{ 
}

nsresult
nsXULMenubarAccessible::GetStateInternal(PRUint32 *aState,
                                         PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  return rv;
}


nsresult
nsXULMenubarAccessible::GetNameInternal(nsAString& aName)
{
  aName.AssignLiteral("Application");
  return NS_OK;
}

nsresult
nsXULMenubarAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_MENUBAR;
  return NS_OK;
}

