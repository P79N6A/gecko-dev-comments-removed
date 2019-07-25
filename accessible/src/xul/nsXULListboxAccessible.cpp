







































#include "nsXULListboxAccessible.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "Role.h"
#include "States.h"

#include "nsComponentManagerUtils.h"
#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompletePopup.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULPopupElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"

using namespace mozilla::a11y;





nsXULColumnsAccessible::
  nsXULColumnsAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

role
nsXULColumnsAccessible::NativeRole()
{
  return roles::LIST;
}

PRUint64
nsXULColumnsAccessible::NativeState()
{
  return states::READONLY;
}






nsXULColumnItemAccessible::
  nsXULColumnItemAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsLeafAccessible(aContent, aShell)
{
}

role
nsXULColumnItemAccessible::NativeRole()
{
  return roles::COLUMNHEADER;
}

PRUint64
nsXULColumnItemAccessible::NativeState()
{
  return states::READONLY;
}

PRUint8
nsXULColumnItemAccessible::ActionCount()
{
  return 1;
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

  DoCommand();
  return NS_OK;
}





nsXULListboxAccessible::
  nsXULListboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  XULSelectControlAccessible(aContent, aShell)
{
  nsIContent* parentContent = mContent->GetParent();
  if (parentContent) {
    nsCOMPtr<nsIAutoCompletePopup> autoCompletePopupElm =
      do_QueryInterface(parentContent);
    if (autoCompletePopupElm)
      mFlags |= eAutoCompletePopupAccessible;
  }
}

NS_IMPL_ADDREF_INHERITED(nsXULListboxAccessible, XULSelectControlAccessible)
NS_IMPL_RELEASE_INHERITED(nsXULListboxAccessible, XULSelectControlAccessible)

nsresult
nsXULListboxAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  nsresult rv = XULSelectControlAccessible::QueryInterface(aIID, aInstancePtr);
  if (*aInstancePtr)
    return rv;

  if (aIID.Equals(NS_GET_IID(nsIAccessibleTable)) && IsMulticolumn()) {
    *aInstancePtr = static_cast<nsIAccessibleTable*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return NS_ERROR_NO_INTERFACE;
}

bool
nsXULListboxAccessible::IsMulticolumn()
{
  PRInt32 numColumns = 0;
  nsresult rv = GetColumnCount(&numColumns);
  if (NS_FAILED(rv))
    return false;

  return numColumns > 1;
}




PRUint64
nsXULListboxAccessible::NativeState()
{
  
  

  
  PRUint64 states = nsAccessible::NativeState();

  

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::seltype,
                            nsGkAtoms::multiple, eCaseMatters)) {
      states |= states::MULTISELECTABLE | states::EXTSELECTABLE;
  }

  return states;
}




NS_IMETHODIMP nsXULListboxAccessible::GetValue(nsAString& _retval)
{
  _retval.Truncate();
  nsCOMPtr<nsIDOMXULSelectControlElement> select(do_QueryInterface(mContent));
  if (select) {
    nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
    select->GetSelectedItem(getter_AddRefs(selectedItem));
    if (selectedItem)
      return selectedItem->GetLabel(_retval);
  }
  return NS_ERROR_FAILURE;
}

role
nsXULListboxAccessible::NativeRole()
{
  
  
  nsCOMPtr<nsIDOMXULPopupElement> xulPopup =
    do_QueryInterface(mContent->GetParent());
  if (xulPopup)
    return roles::COMBOBOX_LIST;

  return IsMulticolumn() ? roles::TABLE : roles::LISTBOX;
}




NS_IMETHODIMP
nsXULListboxAccessible::GetCaption(nsIAccessible **aCaption)
{
  NS_ENSURE_ARG_POINTER(aCaption);
  *aCaption = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSummary(nsAString &aSummary)
{
  aSummary.Truncate();

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetColumnCount(PRInt32 *aColumnsCout)
{
  NS_ENSURE_ARG_POINTER(aColumnsCout);
  *aColumnsCout = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIContent* headContent = nsnull;
  for (nsIContent* childContent = mContent->GetFirstChild(); childContent;
       childContent = childContent->GetNextSibling()) {
    if (childContent->NodeInfo()->Equals(nsGkAtoms::listcols,
                                         kNameSpaceID_XUL)) {
      headContent = childContent;
    }
  }
  if (!headContent)
    return NS_OK;

  PRUint32 columnCount = 0;
  for (nsIContent* childContent = headContent->GetFirstChild(); childContent;
       childContent = childContent->GetNextSibling()) {
    if (childContent->NodeInfo()->Equals(nsGkAtoms::listcol,
                                         kNameSpaceID_XUL)) {
      columnCount++;
    }
  }

  *aColumnsCout = columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRowCount(PRInt32 *aRowCount)
{
  NS_ENSURE_ARG_POINTER(aRowCount);
  *aRowCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULSelectControlElement> element(do_QueryInterface(mContent));
  NS_ENSURE_STATE(element);

  PRUint32 itemCount = 0;
  nsresult rv = element->GetItemCount(&itemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRowCount = itemCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetCellAt(PRInt32 aRow, PRInt32 aColumn,
                                  nsIAccessible **aAccessibleCell)
{
  NS_ENSURE_ARG_POINTER(aAccessibleCell);
  *aAccessibleCell = nsnull;

  if (IsDefunct())
    return NS_OK;

  nsCOMPtr<nsIDOMXULSelectControlElement> control =
    do_QueryInterface(mContent);

  nsCOMPtr<nsIDOMXULSelectControlItemElement> item;
  control->GetItemAtIndex(aRow, getter_AddRefs(item));
  NS_ENSURE_TRUE(item, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIContent> itemContent(do_QueryInterface(item));

  nsAccessible *row =
    GetAccService()->GetAccessibleInWeakShell(itemContent, mWeakShell);
  NS_ENSURE_STATE(row);

  nsresult rv = row->GetChildAt(aColumn, aAccessibleCell);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetCellIndexAt(PRInt32 aRow, PRInt32 aColumn,
                                       PRInt32 *aIndex)
{
  NS_ENSURE_ARG_POINTER(aIndex);
  *aIndex = -1;

  PRInt32 rowCount = 0;
  nsresult rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(0 <= aRow && aRow <= rowCount, NS_ERROR_INVALID_ARG);

  PRInt32 columnCount = 0;
  rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(0 <= aColumn && aColumn <= columnCount, NS_ERROR_INVALID_ARG);

  *aIndex = aRow * columnCount + aColumn;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetColumnIndexAt(PRInt32 aIndex, PRInt32 *aColumn)
{
  NS_ENSURE_ARG_POINTER(aColumn);
  *aColumn = -1;

  PRInt32 columnCount = 0;
  nsresult rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aColumn = aIndex % columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRowIndexAt(PRInt32 aIndex, PRInt32 *aRow)
{
  NS_ENSURE_ARG_POINTER(aRow);
  *aRow = -1;

  PRInt32 columnCount = 0;
  nsresult rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRow = aIndex / columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRowAndColumnIndicesAt(PRInt32 aCellIndex,
                                                 PRInt32* aRowIndex,
                                                 PRInt32* aColumnIndex)
{
  NS_ENSURE_ARG_POINTER(aRowIndex);
  *aRowIndex = -1;
  NS_ENSURE_ARG_POINTER(aColumnIndex);
  *aColumnIndex = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 columnCount = 0;
  nsresult rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aColumnIndex = aCellIndex % columnCount;
  *aRowIndex = aCellIndex / columnCount;
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
  aDescription.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetRowDescription(PRInt32 aRow, nsAString& aDescription)
{
  aDescription.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::IsColumnSelected(PRInt32 aColumn, bool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = false;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");

  PRInt32 selectedrowCount = 0;
  nsresult rv = control->GetSelectedCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rowCount = 0;
  rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsSelected = (selectedrowCount == rowCount);
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::IsRowSelected(PRInt32 aRow, bool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = false;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULSelectControlElement.");
  
  nsCOMPtr<nsIDOMXULSelectControlItemElement> item;
  control->GetItemAtIndex(aRow, getter_AddRefs(item));
  NS_ENSURE_TRUE(item, NS_ERROR_INVALID_ARG);

  return item->GetSelected(aIsSelected);
}

NS_IMETHODIMP
nsXULListboxAccessible::IsCellSelected(PRInt32 aRowIndex, PRInt32 aColumnIndex,
                                       bool *aIsSelected)
{
  return IsRowSelected(aRowIndex, aIsSelected);
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedCellCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");

  nsCOMPtr<nsIDOMNodeList> selectedItems;
  control->GetSelectedItems(getter_AddRefs(selectedItems));
  if (!selectedItems)
    return NS_OK;

  PRUint32 selectedItemsCount = 0;
  nsresult rv = selectedItems->GetLength(&selectedItemsCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!selectedItemsCount)
    return NS_OK;

  PRInt32 columnCount = 0;
  rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aCount = selectedItemsCount * columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedColumnCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");

  PRInt32 selectedrowCount = 0;
  nsresult rv = control->GetSelectedCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rowCount = 0;
  rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (selectedrowCount != rowCount)
    return NS_OK;

  PRInt32 columnCount = 0;
  rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aCount = columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedRowCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");

  PRInt32 selectedrowCount = 0;
  nsresult rv = control->GetSelectedCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aCount = selectedrowCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedCells(nsIArray **aCells)
{
  NS_ENSURE_ARG_POINTER(aCells);
  *aCells = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIMutableArray> selCells =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");

  nsCOMPtr<nsIDOMNodeList> selectedItems;
  control->GetSelectedItems(getter_AddRefs(selectedItems));
  if (!selectedItems)
    return NS_OK;

  PRUint32 selectedItemsCount = 0;
  rv = selectedItems->GetLength(&selectedItemsCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 index = 0;
  for (; index < selectedItemsCount; index++) {
    nsCOMPtr<nsIDOMNode> itemNode;
    selectedItems->Item(index, getter_AddRefs(itemNode));
    nsCOMPtr<nsIContent> itemContent(do_QueryInterface(itemNode));
    nsAccessible *item =
      GetAccService()->GetAccessibleInWeakShell(itemContent, mWeakShell);

    if (item) {
      PRInt32 cellCount = item->GetChildCount();
      for (PRInt32 cellIdx = 0; cellIdx < cellCount; cellIdx++) {
        nsAccessible *cell = mChildren[cellIdx];
        if (cell->Role() == roles::CELL)
          selCells->AppendElement(static_cast<nsIAccessible*>(cell), false);
      }
    }
  }

  NS_ADDREF(*aCells = selCells);
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedCellIndices(PRUint32 *aNumCells,
                                               PRInt32 **aCells)
{
  NS_ENSURE_ARG_POINTER(aNumCells);
  *aNumCells = 0;
  NS_ENSURE_ARG_POINTER(aCells);
  *aCells = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");

  nsCOMPtr<nsIDOMNodeList> selectedItems;
  control->GetSelectedItems(getter_AddRefs(selectedItems));
  if (!selectedItems)
    return NS_OK;

  PRUint32 selectedItemsCount = 0;
  nsresult rv = selectedItems->GetLength(&selectedItemsCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 columnCount = 0;
  rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 cellsCount = selectedItemsCount * columnCount;

  PRInt32 *cellsIdxArray =
    static_cast<PRInt32*>(nsMemory::Alloc((cellsCount) * sizeof(PRInt32)));
  NS_ENSURE_TRUE(cellsIdxArray, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 index = 0, cellsIdx = 0;
  for (; index < selectedItemsCount; index++) {
    nsCOMPtr<nsIDOMNode> itemNode;
    selectedItems->Item(index, getter_AddRefs(itemNode));
    nsCOMPtr<nsIDOMXULSelectControlItemElement> item =
      do_QueryInterface(itemNode);

    if (item) {
      PRInt32 itemIdx = -1;
      control->GetIndexOfItem(item, &itemIdx);
      if (itemIdx != -1) {
        PRInt32 colIdx = 0;
        for (; colIdx < columnCount; colIdx++)
          cellsIdxArray[cellsIdx++] = itemIdx * columnCount + colIdx;
      }
    }
  }

  *aNumCells = cellsCount;
  *aCells = cellsIdxArray;

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedColumnIndices(PRUint32 *aNumColumns,
                                                 PRInt32 **aColumns)
{
  NS_ENSURE_ARG_POINTER(aNumColumns);
  *aNumColumns = 0;
  NS_ENSURE_ARG_POINTER(aColumns);
  *aColumns = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint32 columnCount = 0;
  nsresult rv = GetSelectedColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!columnCount)
    return NS_OK;

  PRInt32 *colsIdxArray =
    static_cast<PRInt32*>(nsMemory::Alloc((columnCount) * sizeof(PRInt32)));
  NS_ENSURE_TRUE(colsIdxArray, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 colIdx = 0;
  for (; colIdx < columnCount; colIdx++)
    colsIdxArray[colIdx] = colIdx;

  *aNumColumns = columnCount;
  *aColumns = colsIdxArray;

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::GetSelectedRowIndices(PRUint32 *aNumRows,
                                              PRInt32 **aRows)
{
  NS_ENSURE_ARG_POINTER(aNumRows);
  *aNumRows = 0;
  NS_ENSURE_ARG_POINTER(aRows);
  *aRows = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");
  
  nsCOMPtr<nsIDOMNodeList> selectedItems;
  control->GetSelectedItems(getter_AddRefs(selectedItems));
  if (!selectedItems)
    return NS_OK;
  
  PRUint32 selectedItemsCount = 0;
  nsresult rv = selectedItems->GetLength(&selectedItemsCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!selectedItemsCount)
    return NS_OK;

  PRInt32 *rowsIdxArray =
    static_cast<PRInt32*>(nsMemory::Alloc((selectedItemsCount) * sizeof(PRInt32)));
  NS_ENSURE_TRUE(rowsIdxArray, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 index = 0;
  for (; index < selectedItemsCount; index++) {
    nsCOMPtr<nsIDOMNode> itemNode;
    selectedItems->Item(index, getter_AddRefs(itemNode));
    nsCOMPtr<nsIDOMXULSelectControlItemElement> item =
      do_QueryInterface(itemNode);
    
    if (item) {
      PRInt32 itemIdx = -1;
      control->GetIndexOfItem(item, &itemIdx);
      if (itemIdx != -1)
        rowsIdxArray[index] = itemIdx;
    }
  }

  *aNumRows = selectedItemsCount;
  *aRows = rowsIdxArray;

  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::SelectRow(PRInt32 aRow)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");

  nsCOMPtr<nsIDOMXULSelectControlItemElement> item;
  control->GetItemAtIndex(aRow, getter_AddRefs(item));
  NS_ENSURE_TRUE(item, NS_ERROR_INVALID_ARG);

  return control->SelectItem(item);
}

NS_IMETHODIMP
nsXULListboxAccessible::SelectColumn(PRInt32 aColumn)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::UnselectRow(PRInt32 aRow)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> control =
    do_QueryInterface(mContent);
  NS_ASSERTION(control,
               "Doesn't implement nsIDOMXULMultiSelectControlElement.");

  nsCOMPtr<nsIDOMXULSelectControlItemElement> item;
  control->GetItemAtIndex(aRow, getter_AddRefs(item));
  NS_ENSURE_TRUE(item, NS_ERROR_INVALID_ARG);

  return control->RemoveItemFromSelection(item);
}

NS_IMETHODIMP
nsXULListboxAccessible::UnselectColumn(PRInt32 aColumn)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsXULListboxAccessible::IsProbablyForLayout(bool *aIsProbablyForLayout)
{
  NS_ENSURE_ARG_POINTER(aIsProbablyForLayout);
  *aIsProbablyForLayout = false;

  return NS_OK;
}




bool
nsXULListboxAccessible::IsWidget() const
{
  return true;
}

bool
nsXULListboxAccessible::IsActiveWidget() const
{
  if (IsAutoCompletePopup()) {
    nsCOMPtr<nsIAutoCompletePopup> autoCompletePopupElm =
      do_QueryInterface(mContent->GetParent());

    if (autoCompletePopupElm) {
      bool isOpen = false;
      autoCompletePopupElm->GetPopupOpen(&isOpen);
      return isOpen;
    }
  }
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
nsXULListboxAccessible::AreItemsOperable() const
{
  if (IsAutoCompletePopup()) {
    nsCOMPtr<nsIAutoCompletePopup> autoCompletePopupElm =
      do_QueryInterface(mContent->GetParent());

    if (autoCompletePopupElm) {
      bool isOpen = false;
      autoCompletePopupElm->GetPopupOpen(&isOpen);
      return isOpen;
    }
  }
  return true;
}

nsAccessible*
nsXULListboxAccessible::ContainerWidget() const
{
  if (IsAutoCompletePopup()) {
    
    
    
    
    nsCOMPtr<nsIDOMXULMenuListElement> menuListElm =
      do_QueryInterface(mContent->GetParent());
    if (menuListElm) {
      nsCOMPtr<nsIDOMNode> inputElm;
      menuListElm->GetInputField(getter_AddRefs(inputElm));
      if (inputElm) {
        nsCOMPtr<nsINode> inputNode = do_QueryInterface(inputElm);
        if (inputNode) {
          nsAccessible* input = GetAccService()->GetAccessible(inputNode);
          return input ? input->ContainerWidget() : nsnull;
        }
      }
    }
  }
  return nsnull;
}





nsXULListitemAccessible::
  nsXULListitemAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULMenuitemAccessible(aContent, aShell)
{
  mIsCheckbox = mContent->AttrValueIs(kNameSpaceID_None,
                                      nsGkAtoms::type,
                                      nsGkAtoms::checkbox,
                                      eCaseMatters);
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULListitemAccessible, nsAccessible)

nsAccessible *
nsXULListitemAccessible::GetListAccessible()
{
  if (IsDefunct())
    return nsnull;
  
  nsCOMPtr<nsIDOMXULSelectControlItemElement> listItem =
    do_QueryInterface(mContent);
  if (!listItem)
    return nsnull;

  nsCOMPtr<nsIDOMXULSelectControlElement> list;
  listItem->GetControl(getter_AddRefs(list));

  nsCOMPtr<nsIContent> listContent(do_QueryInterface(list));
  if (!listContent)
    return nsnull;

  return GetAccService()->GetAccessibleInWeakShell(listContent, mWeakShell);
}




void
nsXULListitemAccessible::Description(nsString& aDesc)
{
  nsAccessibleWrap::Description(aDesc);
}








nsresult
nsXULListitemAccessible::GetNameInternal(nsAString& aName)
{
  nsIContent* childContent = mContent->GetFirstChild();
  if (childContent) {
    if (childContent->NodeInfo()->Equals(nsGkAtoms::listcell,
                                         kNameSpaceID_XUL)) {
      childContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, aName);
      return NS_OK;
    }
  }
  return GetXULName(aName);
}

role
nsXULListitemAccessible::NativeRole()
{
  nsAccessible *list = GetListAccessible();
  if (!list) {
    NS_ERROR("No list accessible for listitem accessible!");
    return roles::NOTHING;
  }

  if (list->Role() == roles::TABLE)
    return roles::ROW;

  if (mIsCheckbox)
    return roles::CHECKBUTTON;

  if (mParent && mParent->Role() == roles::COMBOBOX_LIST)
    return roles::COMBOBOX_OPTION;

  return roles::RICH_OPTION;
}

PRUint64
nsXULListitemAccessible::NativeState()
{
  if (mIsCheckbox)
    return nsXULMenuitemAccessible::NativeState();

  PRUint64 states = states::FOCUSABLE | states::SELECTABLE;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> listItem =
    do_QueryInterface(mContent);

  if (listItem) {
    bool isSelected;
    listItem->GetSelected(&isSelected);
    if (isSelected)
      states |= states::SELECTED;

    if (FocusMgr()->IsFocused(this))
      states |= states::FOCUSED;
  }

  return states;
}

NS_IMETHODIMP nsXULListitemAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click && mIsCheckbox) {
    
    PRUint64 states = NativeState();

    if (states & states::CHECKED)
      aName.AssignLiteral("uncheck");
    else
      aName.AssignLiteral("check");

    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

bool
nsXULListitemAccessible::GetAllowsAnonChildAccessibles()
{
  
  return true;
}

void
nsXULListitemAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                    PRInt32 *aSetSize)
{
  nsAccUtils::GetPositionAndSizeForXULSelectControlItem(mContent, aPosInSet,
                                                        aSetSize);
}




nsAccessible*
nsXULListitemAccessible::ContainerWidget() const
{
  return Parent();
}






nsXULListCellAccessible::
  nsXULListCellAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}




NS_IMPL_ISUPPORTS_INHERITED1(nsXULListCellAccessible,
                             nsHyperTextAccessible,
                             nsIAccessibleTableCell)




NS_IMETHODIMP
nsXULListCellAccessible::GetTable(nsIAccessibleTable **aTable)
{
  NS_ENSURE_ARG_POINTER(aTable);
  *aTable = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAccessible* thisRow = Parent();
  if (!thisRow || thisRow->Role() != roles::ROW)
    return NS_OK;

  nsAccessible* table = thisRow->Parent();
  if (!table || table->Role() != roles::TABLE)
    return NS_OK;

  CallQueryInterface(table, aTable);
  return NS_OK;
}

NS_IMETHODIMP
nsXULListCellAccessible::GetColumnIndex(PRInt32 *aColumnIndex)
{
  NS_ENSURE_ARG_POINTER(aColumnIndex);
  *aColumnIndex = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAccessible* row = Parent();
  if (!row)
    return NS_OK;

  *aColumnIndex = 0;

  PRInt32 indexInRow = IndexInParent();
  for (PRInt32 idx = 0; idx < indexInRow; idx++) {
    nsAccessible* cell = row->GetChildAt(idx);
    roles::Role role = cell->Role();
    if (role == roles::CELL || role == roles::GRID_CELL ||
        role == roles::ROWHEADER || role == roles::COLUMNHEADER)
      (*aColumnIndex)++;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULListCellAccessible::GetRowIndex(PRInt32 *aRowIndex)
{
  NS_ENSURE_ARG_POINTER(aRowIndex);
  *aRowIndex = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAccessible* row = Parent();
  if (!row)
    return NS_OK;

  nsAccessible* table = row->Parent();
  if (!table)
    return NS_OK;

  *aRowIndex = 0;

  PRInt32 indexInTable = row->IndexInParent();
  for (PRInt32 idx = 0; idx < indexInTable; idx++) {
    row = table->GetChildAt(idx);
    if (row->Role() == roles::ROW)
      (*aRowIndex)++;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULListCellAccessible::GetColumnExtent(PRInt32 *aExtentCount)
{
  NS_ENSURE_ARG_POINTER(aExtentCount);
  *aExtentCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aExtentCount = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListCellAccessible::GetRowExtent(PRInt32 *aExtentCount)
{
  NS_ENSURE_ARG_POINTER(aExtentCount);
  *aExtentCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aExtentCount = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXULListCellAccessible::GetColumnHeaderCells(nsIArray **aHeaderCells)
{
  NS_ENSURE_ARG_POINTER(aHeaderCells);
  *aHeaderCells = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIAccessibleTable> table;
  GetTable(getter_AddRefs(table));
  NS_ENSURE_STATE(table); 

  
  nsAccessible *list = nsnull;

  nsRefPtr<nsAccessible> tableAcc(do_QueryObject(table));
  PRInt32 tableChildCount = tableAcc->GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < tableChildCount; childIdx++) {
    nsAccessible *child = tableAcc->GetChildAt(childIdx);
    if (child->Role() == roles::LIST) {
      list = child;
      break;
    }
  }

  if (list) {
    PRInt32 colIdx = -1;
    GetColumnIndex(&colIdx);

    nsIAccessible *headerCell = list->GetChildAt(colIdx);
    if (headerCell) {
      nsresult rv = NS_OK;
      nsCOMPtr<nsIMutableArray> headerCells =
        do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      headerCells->AppendElement(headerCell, false);
      NS_ADDREF(*aHeaderCells = headerCells);
      return NS_OK;
    }
  }

  
  return nsAccUtils::GetHeaderCellsFor(table, this,
                                       nsAccUtils::eColumnHeaderCells,
                                       aHeaderCells);
}

NS_IMETHODIMP
nsXULListCellAccessible::GetRowHeaderCells(nsIArray **aHeaderCells)
{
  NS_ENSURE_ARG_POINTER(aHeaderCells);
  *aHeaderCells = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIAccessibleTable> table;
  GetTable(getter_AddRefs(table));
  NS_ENSURE_STATE(table); 

  
  return nsAccUtils::GetHeaderCellsFor(table, this,
                                       nsAccUtils::eRowHeaderCells,
                                       aHeaderCells);
}

NS_IMETHODIMP
nsXULListCellAccessible::IsSelected(bool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = false;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIAccessibleTable> table;
  GetTable(getter_AddRefs(table));
  NS_ENSURE_STATE(table); 

  PRInt32 rowIdx = -1;
  GetRowIndex(&rowIdx);

  return table->IsRowSelected(rowIdx, aIsSelected);
}




role
nsXULListCellAccessible::NativeRole()
{
  return roles::CELL;
}

nsresult
nsXULListCellAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIAccessibleTable> table;
  GetTable(getter_AddRefs(table));
  NS_ENSURE_STATE(table); 

  PRInt32 rowIdx = -1;
  GetRowIndex(&rowIdx);
  PRInt32 colIdx = -1;
  GetColumnIndex(&colIdx);

  PRInt32 cellIdx = -1;
  table->GetCellIndexAt(rowIdx, colIdx, &cellIdx);

  nsAutoString stringIdx;
  stringIdx.AppendInt(cellIdx);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::tableCellIndex,
                         stringIdx);

  return NS_OK;
}
