





































#include "nsXULMenuAccessible.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIMutableArray.h"
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







nsXULSelectableAccessible::nsXULSelectableAccessible(nsIDOMNode* aDOMNode,
                                                     nsIWeakReference* aShell):
nsAccessibleWrap(aDOMNode, aShell)
{
  mSelectControl = do_QueryInterface(aDOMNode);
}

NS_IMPL_ISUPPORTS_INHERITED1(nsXULSelectableAccessible, nsAccessible, nsIAccessibleSelectable)

NS_IMETHODIMP nsXULSelectableAccessible::Shutdown()
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

NS_IMETHODIMP nsXULMenuitemAccessible::Init()
{
  nsresult rv = nsAccessibleWrap::Init();
  nsXULMenupopupAccessible::GenerateMenu(mDOMNode);
  return rv;
}

NS_IMETHODIMP nsXULMenuitemAccessible::GetState(PRUint32 *_retval)
{
  nsAccessible::GetState(_retval);

  
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  if (!element)
    return NS_ERROR_FAILURE;
  PRBool isFocused = PR_FALSE;
  element->HasAttribute(NS_LITERAL_STRING("_moz-menuactive"), &isFocused); 
  if (isFocused)
    *_retval |= nsIAccessibleStates::STATE_FOCUSED;

  
  nsAutoString tagName;
  element->GetLocalName(tagName);
  if (tagName.EqualsLiteral("menu")) {
    *_retval |= nsIAccessibleStates::STATE_HASPOPUP;
    PRBool isOpen;
    element->HasAttribute(NS_LITERAL_STRING("open"), &isOpen);
    *_retval |= isOpen ? nsIAccessibleStates::STATE_EXPANDED :
                         nsIAccessibleStates::STATE_COLLAPSED;
  }

  nsAutoString menuItemType;
  element->GetAttribute(NS_LITERAL_STRING("type"), menuItemType); 

  if (!menuItemType.IsEmpty()) {
    
    if (menuItemType.EqualsIgnoreCase("radio") ||
        menuItemType.EqualsIgnoreCase("checkbox"))
      *_retval |= nsIAccessibleStates::STATE_CHECKABLE;

    
    nsAutoString checkValue;
    element->GetAttribute(NS_LITERAL_STRING("checked"), checkValue);
    if (checkValue.EqualsLiteral("true")) {
      *_retval |= nsIAccessibleStates::STATE_CHECKED;
    }
  }

  
  
  
  nsCOMPtr<nsIAccessible> parentAccessible(GetParent());
  if (parentAccessible) {
    *_retval &= ~nsIAccessibleStates::STATE_OFFSCREEN;  
    *_retval |= (State(parentAccessible) & nsIAccessibleStates::STATE_OFFSCREEN);  
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULMenuitemAccessible::GetName(nsAString& _retval)
{
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  if (!element) {
    return NS_ERROR_FAILURE;
  }
  element->GetAttribute(NS_LITERAL_STRING("label"), _retval); 

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


NS_IMETHODIMP nsXULMenuitemAccessible::GetKeyboardShortcut(nsAString& _retval)
{
  static PRInt32 gMenuAccesskeyModifier = -1;  

  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(mDOMNode));
  if (elt) {
    nsAutoString accesskey;
    elt->GetAttribute(NS_LITERAL_STRING("accesskey"), accesskey);
    if (accesskey.IsEmpty())
      return NS_OK;

    nsCOMPtr<nsIAccessible> parentAccessible(GetParent());
    if (parentAccessible) {
      PRUint32 role;
      parentAccessible->GetRole(&role);
      if (role == nsIAccessibleRole::ROLE_MENUBAR) {
        
        
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
          nsAccessible::GetFullKeyName(propertyKey, accesskey, _retval);
      }
    }
    if (_retval.IsEmpty())
      _retval = accesskey;
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

NS_IMETHODIMP nsXULMenuitemAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_MENUITEM;
  if (mParent && Role(mParent) == nsIAccessibleRole::ROLE_COMBOBOX_LIST) {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LISTITEM;
    return NS_OK;
  }
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  if (!element)
    return NS_ERROR_FAILURE;
  nsAutoString menuItemType;
  element->GetAttribute(NS_LITERAL_STRING("type"), menuItemType);
  if (menuItemType.EqualsIgnoreCase("radio"))
    *aRole = nsIAccessibleRole::ROLE_RADIO_MENU_ITEM;
  else if (menuItemType.EqualsIgnoreCase("checkbox"))
    *aRole = nsIAccessibleRole::ROLE_CHECK_MENU_ITEM;
  else { 
    PRInt32 childCount;
    GetChildCount(&childCount);
    if (childCount > 0) {
      *aRole = nsIAccessibleRole::ROLE_PARENT_MENUITEM;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULMenuitemAccessible::GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren)
{
  
  *aAllowsAnonChildren = PR_FALSE;
  return NS_OK;
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

NS_IMETHODIMP nsXULMenuSeparatorAccessible::GetState(PRUint32 *_retval)
{
  
  nsXULMenuitemAccessible::GetState(_retval);
  *_retval &= nsIAccessibleStates::STATE_OFFSCREEN;

  return NS_OK;
}

NS_IMETHODIMP nsXULMenuSeparatorAccessible::GetName(nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP nsXULMenuSeparatorAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_SEPARATOR;
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

NS_IMETHODIMP nsXULMenupopupAccessible::GetState(PRUint32 *_retval)
{
  
  *_retval = 0;
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

  if (!isActive)
    *_retval |= nsIAccessibleStates::STATE_OFFSCREEN;

  return NS_OK;
}

already_AddRefed<nsIDOMNode>
nsXULMenupopupAccessible::FindInNodeList(nsIDOMNodeList *aNodeList, 
                                         nsIAtom *aAtom, PRUint32 aNameSpaceID)
{
  PRUint32 numChildren;
  if (!aNodeList || NS_FAILED(aNodeList->GetLength(&numChildren))) {
    return nsnull;
  }
  nsCOMPtr<nsIDOMNode> childNode;
  for (PRUint32 childIndex = 0; childIndex < numChildren; childIndex++) {
    aNodeList->Item(childIndex, getter_AddRefs(childNode));
    nsCOMPtr<nsIContent> content = do_QueryInterface(childNode);
    if (content && content->NodeInfo()->Equals(aAtom, kNameSpaceID_XUL)) {
      nsIDOMNode *matchNode = childNode;
      NS_ADDREF(matchNode);
      return matchNode;
    }
  }
  return nsnull;
}

void nsXULMenupopupAccessible::GenerateMenu(nsIDOMNode *aNode)
{
  
  
  nsCOMPtr<nsIDOMNodeList> nodeList;
  aNode->GetChildNodes(getter_AddRefs(nodeList));

  nsCOMPtr<nsIDOMNode> menuPopup = FindInNodeList(nodeList, nsAccessibilityAtoms::menupopup,
                                                  kNameSpaceID_XUL);
  nsCOMPtr<nsIDOMElement> popupElement(do_QueryInterface(menuPopup));
  if (popupElement) {
    nsAutoString attr;
    popupElement->GetAttribute(NS_LITERAL_STRING("menugenerated"), attr);
    if (!attr.EqualsLiteral("true")) {
      popupElement->SetAttribute(NS_LITERAL_STRING("menugenerated"), NS_LITERAL_STRING("true"));
    }
  }
}

NS_IMETHODIMP nsXULMenupopupAccessible::GetName(nsAString& _retval)
{
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(mDOMNode));
  NS_ASSERTION(element, "No element for popup node!");

  while (element) {
    element->GetAttribute(NS_LITERAL_STRING("label"), _retval);
    if (!_retval.IsEmpty())
      return NS_OK;
    nsCOMPtr<nsIDOMNode> parentNode, node(do_QueryInterface(element));
    if (!node)
      return NS_ERROR_FAILURE;
    node->GetParentNode(getter_AddRefs(parentNode));
    element = do_QueryInterface(parentNode);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULMenupopupAccessible::GetRole(PRUint32 *aRole)
{
  if (mParent && Role(mParent) == nsIAccessibleRole::ROLE_COMBOBOX) {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
  }
  else {
    *aRole = nsIAccessibleRole::ROLE_MENUPOPUP;
  }
  return NS_OK;
}



nsXULMenubarAccessible::nsXULMenubarAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell): 
  nsAccessibleWrap(aDOMNode, aShell)
{ 
}

NS_IMETHODIMP nsXULMenubarAccessible::GetState(PRUint32 *_retval)
{
  nsresult rv = nsAccessible::GetState(_retval);
  *_retval &= ~nsIAccessibleStates::STATE_FOCUSABLE; 
  return rv;
}


NS_IMETHODIMP nsXULMenubarAccessible::GetName(nsAString& _retval)
{
  _retval.AssignLiteral("Application");

  return NS_OK;
}

NS_IMETHODIMP nsXULMenubarAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_MENUBAR;
  return NS_OK;
}

