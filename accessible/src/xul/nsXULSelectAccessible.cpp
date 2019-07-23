






































#include "nsXULSelectAccessible.h"
#include "nsAccessibilityService.h"
#include "nsIContent.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULPopupElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULTextboxElement.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsCaseTreatment.h"




nsXULColumnsAccessible::
  nsXULColumnsAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell) :
  nsAccessibleWrap(aDOMNode, aShell)
{
}

NS_IMETHODIMP
nsXULColumnsAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_LIST;
  return NS_OK;
}

NS_IMETHODIMP
nsXULColumnsAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);
  *aState = nsIAccessibleStates::STATE_READONLY;
  if (aExtraState) {
    *aExtraState = mDOMNode ? 0 : nsIAccessibleStates::EXT_STATE_DEFUNCT ;
  }
  return NS_OK;
}




nsXULColumnItemAccessible::
  nsXULColumnItemAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell) :
  nsLeafAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP
nsXULColumnItemAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_COLUMNHEADER;
  return NS_OK;
}

NS_IMETHODIMP
nsXULColumnItemAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);
  *aState = nsIAccessibleStates::STATE_READONLY;
  if (aExtraState) {
    *aExtraState = mDOMNode ? 0 : nsIAccessibleStates::EXT_STATE_DEFUNCT ;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXULColumnItemAccessible::GetName(nsAString& aName)
{
  return GetXULName(aName);
}

NS_IMETHODIMP
nsXULColumnItemAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);

  *aNumActions = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXULColumnItemAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("click");
  return NS_OK;
}

NS_IMETHODIMP
nsXULColumnItemAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  return DoCommand();
}




nsXULListboxAccessible::
  nsXULListboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell) :
  nsXULSelectableAccessible(aDOMNode, aShell)
{
}

NS_IMPL_ADDREF_INHERITED(nsXULListboxAccessible, nsXULSelectableAccessible)
NS_IMPL_RELEASE_INHERITED(nsXULListboxAccessible, nsXULSelectableAccessible)

nsresult
nsXULListboxAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  nsresult rv = nsXULSelectableAccessible::QueryInterface(aIID, aInstancePtr);
  if (*aInstancePtr)
    return rv;

  if (aIID.Equals(NS_GET_IID(nsIAccessibleTable)) && IsTree()) {
    *aInstancePtr = static_cast<nsIAccessibleTable*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return NS_ERROR_NO_INTERFACE;
}

PRBool
nsXULListboxAccessible::IsTree()
{
  PRInt32 numColumns = 0;
  nsresult rv = GetColumns(&numColumns);
  if (NS_FAILED(rv))
    return PR_FALSE;
  
  return numColumns > 1;
}




NS_IMETHODIMP
nsXULListboxAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  
  
  

  
  nsresult rv = nsAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mDOMNode) {
    return NS_OK;
  }


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

NS_IMETHODIMP nsXULListboxAccessible::GetRole(PRUint32 *aRole)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
  if (content) {
    
    
    nsCOMPtr<nsIDOMXULPopupElement> xulPopup =
      do_QueryInterface(content->GetParent());
    if (xulPopup) {
      *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
      return NS_OK;
    }
  }

  if (IsTree())
    *aRole = nsIAccessibleRole::ROLE_TABLE;
  else
    *aRole = nsIAccessibleRole::ROLE_LISTBOX;

  return NS_OK;
}




NS_IMETHODIMP
nsXULListboxAccessible::GetCaption(nsIAccessible **aCaption)
{
  NS_ENSURE_ARG_POINTER(aCaption);
  *aCaption = nsnull;

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSummary(nsAString &aSummary)
{
  aSummary.Truncate();

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetColumns(PRInt32 *aNumColumns)
{
  NS_ENSURE_ARG_POINTER(aNumColumns);
  *aNumColumns = 0;

  if (!mDOMNode)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  nsCOMPtr<nsIContent> headContent;
  PRUint32 count = content->GetChildCount();

  for (PRUint32 index = 0; index < count; ++index) {
    nsCOMPtr<nsIContent> childContent(content->GetChildAt(index));
    NS_ENSURE_STATE(childContent);

    if (childContent->NodeInfo()->Equals(nsAccessibilityAtoms::listhead,
                                         kNameSpaceID_XUL)) {
      headContent = childContent;
    }
  }

  if (!headContent)
    return NS_OK;

  PRUint32 columnCount = 0;
  count = headContent->GetChildCount();

  for (PRUint32 index = 0; index < count; ++index) {
    nsCOMPtr<nsIContent> childContent(headContent->GetChildAt(index));
    NS_ENSURE_STATE(childContent);

    if (childContent->NodeInfo()->Equals(nsAccessibilityAtoms::listheader,
                                         kNameSpaceID_XUL)) {
      columnCount++;
    }
  }

  *aNumColumns = columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetColumnHeader(nsIAccessibleTable **aColumnHeader)
{
  NS_ENSURE_ARG_POINTER(aColumnHeader);
  *aColumnHeader = nsnull;

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRows(PRInt32 *aNumRows)
{
  NS_ENSURE_ARG_POINTER(aNumRows);
  *aNumRows = 0;

  if (!mDOMNode)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULSelectControlElement> element(do_QueryInterface(mDOMNode));
  NS_ENSURE_STATE(element);

  PRUint32 itemCount = 0;
  nsresult rv = element->GetItemCount(&itemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aNumRows = itemCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRowHeader(nsIAccessibleTable **aRowHeader)
{
  NS_ENSURE_ARG_POINTER(aRowHeader);
  *aRowHeader = nsnull;

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::CellRefAt(PRInt32 aRow, PRInt32 aColumn,
                                  nsIAccessible **aAccessibleCell)
{
  NS_ENSURE_ARG_POINTER(aAccessibleCell);
  *aAccessibleCell = nsnull;

  if (IsDefunct())
    return NS_OK;

  nsCOMPtr<nsIDOMXULSelectControlElement> control =
    do_QueryInterface(mDOMNode);

  nsCOMPtr<nsIDOMXULSelectControlItemElement> item;
  control->GetItemAtIndex(aRow, getter_AddRefs(item));
  NS_ENSURE_TRUE(item, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIAccessible> accessibleRow;
  GetAccService()->GetAccessibleInWeakShell(item, mWeakShell,
                                            getter_AddRefs(accessibleRow));
  NS_ENSURE_STATE(accessibleRow);

  nsresult rv = accessibleRow->GetChildAt(aColumn, aAccessibleCell);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetIndexAt(PRInt32 aRow, PRInt32 aColumn,
                                   PRInt32 *aIndex)
{
  NS_ENSURE_ARG_POINTER(aIndex);
  *aIndex = -1;

  PRInt32 rowCount = 0;
  nsresult rv = GetRows(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(0 <= aRow && aRow <= rowCount, NS_ERROR_INVALID_ARG);

  PRInt32 columnCount = 0;
  rv = GetColumns(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(0 <= aColumn && aColumn <= columnCount, NS_ERROR_INVALID_ARG);

  *aIndex = aRow * columnCount + aColumn;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetColumnAtIndex(PRInt32 aIndex, PRInt32 *aColumn)
{
  NS_ENSURE_ARG_POINTER(aColumn);
  *aColumn = -1;

  PRInt32 columnCount = 0;
  nsresult rv = GetColumns(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aColumn = aIndex % columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRowAtIndex(PRInt32 aIndex, PRInt32 *aRow)
{
  NS_ENSURE_ARG_POINTER(aRow);
  *aRow = -1;

  PRInt32 columnCount = 0;
  nsresult rv = GetColumns(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRow = aIndex / columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetColumnExtentAt(PRInt32 aRow, PRInt32 aColumn,
                                          PRInt32 *aCellSpans)
{
  NS_ENSURE_ARG_POINTER(aCellSpans);
  *aCellSpans = 1;

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRowExtentAt(PRInt32 aRow, PRInt32 aColumn,
                                       PRInt32 *aCellSpans)
{
  NS_ENSURE_ARG_POINTER(aCellSpans);
  *aCellSpans = 1;

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetColumnDescription(PRInt32 aColumn,
                                             nsAString& aDescription)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRowDescription(PRInt32 aRow, nsAString& aDescription)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::IsColumnSelected(PRInt32 aColumn, PRBool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::IsRowSelected(PRInt32 aRow, PRBool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::IsCellSelected(PRInt32 aRow, PRInt32 aColumn,
                                       PRBool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedCellsCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedColumnsCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedRowsCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedCells(PRUint32 *aNumCells, PRInt32 **aCells)
{
  NS_ENSURE_ARG_POINTER(aNumCells);
  NS_ENSURE_ARG_POINTER(aCells);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedColumns(PRUint32 *aNumColumns,
                                           PRInt32 **aColumns)
{
  NS_ENSURE_ARG_POINTER(aNumColumns);
  NS_ENSURE_ARG_POINTER(aColumns);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedRows(PRUint32 *aNumRows, PRInt32 **aRows)
{
  NS_ENSURE_ARG_POINTER(aNumRows);
  NS_ENSURE_ARG_POINTER(aRows);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::SelectRow(PRInt32 aRow)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::SelectColumn(PRInt32 aColumn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::UnselectRow(PRInt32 aRow)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::UnselectColumn(PRInt32 aColumn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULListboxAccessible::IsProbablyForLayout(PRBool *aIsProbablyForLayout)
{
  NS_ENSURE_ARG_POINTER(aIsProbablyForLayout);
  *aIsProbablyForLayout = PR_FALSE;

  return NS_OK;
}




nsXULListitemAccessible::
  nsXULListitemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell) :
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

already_AddRefed<nsIAccessible>
nsXULListitemAccessible::GetListAccessible()
{
  if (IsDefunct())
    return nsnull;
  
  nsCOMPtr<nsIDOMXULSelectControlItemElement> listItem =
    do_QueryInterface(mDOMNode);
  
  nsCOMPtr<nsIDOMXULSelectControlElement> list;
  listItem->GetControl(getter_AddRefs(list));
  if (!list)
    return nsnull;

  nsIAccessible *listAcc = nsnull;
  GetAccService()->GetAccessibleInWeakShell(list, mWeakShell, &listAcc);
  return listAcc;
}








NS_IMETHODIMP nsXULListitemAccessible::GetName(nsAString& _retval)
{
  if (!mDOMNode)
    return NS_ERROR_FAILURE;

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
  nsCOMPtr<nsIAccessible> listAcc = GetListAccessible();
  NS_ENSURE_STATE(listAcc);

  if (Role(listAcc) == nsIAccessibleRole::ROLE_TABLE) {
    *aRole = nsIAccessibleRole::ROLE_ROW;
    return NS_OK;
  }

  if (mIsCheckbox)
    *aRole = nsIAccessibleRole::ROLE_CHECKBUTTON;
  else if (mParent && Role(mParent) == nsIAccessibleRole::ROLE_COMBOBOX_LIST)
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_OPTION;
  else
    *aRole = nsIAccessibleRole::ROLE_RICH_OPTION;
  return NS_OK;
}




NS_IMETHODIMP
nsXULListitemAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  if (mIsCheckbox) {
    return nsXULMenuitemAccessible::GetState(aState, aExtraState);
  }

  *aState = 0;
  if (!mDOMNode) {
    if (aExtraState) {
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;
    }
    return NS_OK;
  }
  if (aExtraState)
    *aExtraState = 0;

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

NS_IMETHODIMP
nsXULListitemAccessible::GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren)
{
  
  *aAllowsAnonChildren = PR_TRUE;
  return NS_OK;
}

nsresult
nsXULListitemAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);

  
  
  
  nsresult rv = nsAccessible::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAccUtils::SetAccAttrsForXULSelectControlItem(mDOMNode, aAttributes);
  return NS_OK;
}





nsXULListCellAccessible::
  nsXULListCellAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
  nsHyperTextAccessibleWrap(aDOMNode, aShell)
{
}

NS_IMETHODIMP
nsXULListCellAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_CELL;
  return NS_OK;
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
  if (content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                           NS_LITERAL_STRING("autocomplete"), eIgnoreCase)) {
    *aRole = nsIAccessibleRole::ROLE_AUTOCOMPLETE;
  } else {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX;
  }
  return NS_OK;
}










NS_IMETHODIMP
nsXULComboboxAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  nsresult rv = nsAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mDOMNode) {
    return NS_OK;
  }

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
  if (!mDOMNode)
    return NS_ERROR_FAILURE;

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

