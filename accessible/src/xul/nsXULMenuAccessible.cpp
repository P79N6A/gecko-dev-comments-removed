





































#include "nsXULMenuAccessible.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsXULFormControlAccessible.h"

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
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"


static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);






nsXULSelectableAccessible::
  nsXULSelectableAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
  mSelectControl = do_QueryInterface(aContent);
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
  nsAccessible* child = GetChildAt(aIndex);
  NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> childNode;
  child->GetDOMNode(getter_AddRefs(childNode));
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

  nsCOMPtr<nsIMutableArray> selectedAccessibles =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_STATE(selectedAccessibles);

  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);
  if (xulMultiSelect) {
    PRInt32 length = 0;
    xulMultiSelect->GetSelectedCount(&length);
    for (PRInt32 index = 0; index < length; index++) {
      nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
      xulMultiSelect->GetSelectedItem(index, getter_AddRefs(selectedItem));
      nsCOMPtr<nsIContent> selectedContent(do_QueryInterface(selectedItem));
      nsAccessible *selectedAcc =
        GetAccService()->GetAccessibleInWeakShell(selectedContent, mWeakShell);
      if (selectedAcc)
        selectedAccessibles->AppendElement(static_cast<nsIAccessible*>(selectedAcc),
                                           PR_FALSE);
    }
  }
  else {  
    nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
    mSelectControl->GetSelectedItem(getter_AddRefs(selectedItem));
    nsCOMPtr<nsIContent> selectedContent(do_QueryInterface(selectedItem));
    if(selectedContent) {
      nsAccessible *selectedAcc =
        GetAccService()->GetAccessibleInWeakShell(selectedContent, mWeakShell);
      if (selectedAcc)
        selectedAccessibles->AppendElement(static_cast<nsIAccessible*>(selectedAcc),
                                           PR_FALSE);
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

  if (!selectedItem)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> selectedContent(do_QueryInterface(selectedItem));
  nsAccessible *selectedAcc =
    GetAccService()->GetAccessibleInWeakShell(selectedContent, mWeakShell);
  if (!selectedAcc)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aAccessible = selectedAcc);
  return NS_OK;
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






nsXULMenuitemAccessible::
  nsXULMenuitemAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

nsresult
nsXULMenuitemAccessible::Init()
{
  nsresult rv = nsAccessibleWrap::Init();
  nsCoreUtils::GeneratePopupTree(mContent);
  return rv;
}

nsresult
nsXULMenuitemAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  if (mContent->HasAttr(kNameSpaceID_None,
                        nsAccessibilityAtoms::_moz_menuactive))
    *aState |= nsIAccessibleStates::STATE_FOCUSED;

  
  if (mContent->NodeInfo()->Equals(nsAccessibilityAtoms::menu,
                                   kNameSpaceID_XUL)) {
    *aState |= nsIAccessibleStates::STATE_HASPOPUP;
    if (mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::open))
      *aState |= nsIAccessibleStates::STATE_EXPANDED;
    else
      *aState |= nsIAccessibleStates::STATE_COLLAPSED;
  }

  
  static nsIContent::AttrValuesArray strings[] =
    { &nsAccessibilityAtoms::radio, &nsAccessibilityAtoms::checkbox, nsnull };

  if (mContent->FindAttrValueIn(kNameSpaceID_None,
                                nsAccessibilityAtoms::type,
                                strings, eCaseMatters) >= 0) {

    
    *aState |= nsIAccessibleStates::STATE_CHECKABLE;

    
    if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::checked,
                              nsAccessibilityAtoms::_true, eCaseMatters))
      *aState |= nsIAccessibleStates::STATE_CHECKED;
  }

  
  PRBool isComboboxOption =
    (nsAccUtils::Role(this) == nsIAccessibleRole::ROLE_COMBOBOX_OPTION);
  if (isComboboxOption) {
    
    PRBool isSelected = PR_FALSE;
    nsCOMPtr<nsIDOMXULSelectControlItemElement>
      item(do_QueryInterface(mContent));
    NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);
    item->GetSelected(&isSelected);

    
    PRBool isCollapsed = PR_FALSE;
    nsAccessible* parentAcc = GetParent();
    if (nsAccUtils::State(parentAcc) & nsIAccessibleStates::STATE_INVISIBLE)
      isCollapsed = PR_TRUE;

    if (isSelected) {
      *aState |= nsIAccessibleStates::STATE_SELECTED;

      
      if (isCollapsed) {
        
        nsAccessible* grandParentAcc = parentAcc->GetParent();
        NS_ENSURE_TRUE(grandParentAcc, NS_ERROR_FAILURE);
        NS_ASSERTION(nsAccUtils::Role(grandParentAcc) == nsIAccessibleRole::ROLE_COMBOBOX,
                     "grandparent of combobox listitem is not combobox");
        PRUint32 grandParentState, grandParentExtState;
        grandParentAcc->GetState(&grandParentState, &grandParentExtState);
        *aState &= ~(nsIAccessibleStates::STATE_OFFSCREEN |
                     nsIAccessibleStates::STATE_INVISIBLE);
        *aState |= (grandParentState & nsIAccessibleStates::STATE_OFFSCREEN) |
                   (grandParentState & nsIAccessibleStates::STATE_INVISIBLE);
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
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::label, aName);
  return NS_OK;
}

NS_IMETHODIMP
nsXULMenuitemAccessible::GetDescription(nsAString& aDescription)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::description,
                    aDescription);

  return NS_OK;
}


NS_IMETHODIMP
nsXULMenuitemAccessible::GetKeyboardShortcut(nsAString& aAccessKey)
{
  aAccessKey.Truncate();
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  static PRInt32 gMenuAccesskeyModifier = -1;  

  
  
  nsAutoString accesskey;
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::accesskey,
                    accesskey);
  if (accesskey.IsEmpty())
    return NS_OK;

  nsAccessible* parentAcc = GetParent();
  if (parentAcc) {
    if (nsAccUtils::RoleInternal(parentAcc) == nsIAccessibleRole::ROLE_MENUBAR) {
      
      
      if (gMenuAccesskeyModifier == -1) {
        
        gMenuAccesskeyModifier = 0;
        nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
        if (prefBranch)
          prefBranch->GetIntPref("ui.key.menuAccessKey", &gMenuAccesskeyModifier);
      }

      nsAutoString propertyKey;
      switch (gMenuAccesskeyModifier) {
        case nsIDOMKeyEvent::DOM_VK_CONTROL:
          propertyKey.AssignLiteral("VK_CONTROL");
          break;
        case nsIDOMKeyEvent::DOM_VK_ALT:
          propertyKey.AssignLiteral("VK_ALT");
          break;
        case nsIDOMKeyEvent::DOM_VK_META:
          propertyKey.AssignLiteral("VK_META");
          break;
      }

      if (!propertyKey.IsEmpty())
        nsAccessible::GetFullKeyName(propertyKey, accesskey, aAccessKey);
    }
  }

  if (aAccessKey.IsEmpty())
    aAccessKey = accesskey;

  return NS_OK;
}


NS_IMETHODIMP
nsXULMenuitemAccessible::GetDefaultKeyBinding(nsAString& aKeyBinding)
{
  aKeyBinding.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAutoString accelText;
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::acceltext,
                    aKeyBinding);

  return NS_OK;
}

nsresult
nsXULMenuitemAccessible::GetRoleInternal(PRUint32 *aRole)
{
  nsCOMPtr<nsIDOMXULContainerElement> xulContainer(do_QueryInterface(mContent));
  if (xulContainer) {
    *aRole = nsIAccessibleRole::ROLE_PARENT_MENUITEM;
    return NS_OK;
  }

  if (nsAccUtils::Role(GetParent()) == nsIAccessibleRole::ROLE_COMBOBOX_LIST) {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_OPTION;
    return NS_OK;
  }

  if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                            nsAccessibilityAtoms::radio, eCaseMatters)) {
    *aRole = nsIAccessibleRole::ROLE_RADIO_MENU_ITEM;

  } else if (mContent->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                                   nsAccessibilityAtoms::checkbox,
                                   eCaseMatters)) {
    *aRole = nsIAccessibleRole::ROLE_CHECK_MENU_ITEM;

  } else {
    *aRole = nsIAccessibleRole::ROLE_MENUITEM;
  }

  return NS_OK;
}

PRInt32
nsXULMenuitemAccessible::GetLevelInternal()
{
  return nsAccUtils::GetLevelForXULContainerItem(mContent);
}

void
nsXULMenuitemAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                    PRInt32 *aSetSize)
{
  nsAccUtils::GetPositionAndSizeForXULContainerItem(mContent, aPosInSet,
                                                    aSetSize);
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






nsXULMenuSeparatorAccessible::
  nsXULMenuSeparatorAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULMenuitemAccessible(aContent, aShell)
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






nsXULMenupopupAccessible::
  nsXULMenupopupAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULSelectableAccessible(aContent, aShell)
{ 
  
  mSelectControl = do_QueryInterface(mContent->GetParent());
}

nsresult
nsXULMenupopupAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

#ifdef DEBUG_A11Y
  
  PRBool isActive = mContent->HasAttr(kNameSpaceID_None,
                                      nsAccessibilityAtoms::menuactive);
  if (!isActive) {
    nsAccessible* parent(GetParent());
    NS_ENSURE_STATE(parent);

    nsIContent *parentContent = parnet->GetContent();
    NS_ENSURE_STATE(parentContent);

    isActive = parentContent->HasAttr(kNameSpaceID_None,
                                      nsAccessibilityAtoms::open);
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
  nsIContent *content = mContent;
  while (content && aName.IsEmpty()) {
    content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::label, aName);
    content = content->GetParent();
  }

  return NS_OK;
}

nsresult
nsXULMenupopupAccessible::GetRoleInternal(PRUint32 *aRole)
{
  nsAccessible *parent = GetParent();
  if (parent) {
    PRUint32 role = nsAccUtils::Role(parent);
    if (role == nsIAccessibleRole::ROLE_COMBOBOX ||
        role == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
      *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
      return NS_OK;

    } else if (role == nsIAccessibleRole::ROLE_PUSHBUTTON) {
      
      nsAccessible *grandParent = parent->GetParent();
      if (nsAccUtils::Role(grandParent) == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
        *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
        return NS_OK;
      }
    }
  }

  *aRole = nsIAccessibleRole::ROLE_MENUPOPUP;
  return NS_OK;
}






nsXULMenubarAccessible::
  nsXULMenubarAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
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

