







































#include "CAccessibleTable.h"

#include "Accessible2.h"
#include "AccessibleTable_i.c"

#include "nsIAccessible.h"
#include "nsIAccessibleTable.h"
#include "nsIWinAccessNode.h"
#include "nsAccessNodeWrap.h"

#include "nsCOMPtr.h"
#include "nsString.h"

#define CANT_QUERY_ASSERTION_MSG \
"Subclass of CAccessibleTable doesn't implement nsIAccessibleTable"\



STDMETHODIMP
CAccessibleTable::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleTable == iid) {
    *ppv = static_cast<IAccessibleTable*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleTable::get_accessibleAt(long aRow, long aColumn,
                                   IUnknown **aAccessible)
{
__try {
  *aAccessible = NULL;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsCOMPtr<nsIAccessible> cell;
  tableAcc->CellRefAt(aRow, aColumn, getter_AddRefs(cell));

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(cell));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  nsresult rv = winAccessNode->QueryNativeInterface(IID_IAccessible2,
                                                    &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aAccessible = static_cast<IUnknown*>(instancePtr);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleTable::get_caption(IUnknown **aAccessible)
{
__try {
  *aAccessible = NULL;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsCOMPtr<nsIAccessible> caption;
  tableAcc->GetCaption(getter_AddRefs(caption));

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(caption));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  nsresult rv = winAccessNode->QueryNativeInterface(IID_IAccessible2,
                                                    &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aAccessible = static_cast<IUnknown*>(instancePtr);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleTable::get_childIndex(long aRowIndex, long aColumnIndex,
                                 long *aChildIndex)
{
__try {
  *aChildIndex = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRInt32 childIndex = 0;
  nsresult rv = tableAcc->GetIndexAt(aRowIndex, aColumnIndex, &childIndex);
  *aChildIndex = childIndex;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_columnDescription(long aColumn, BSTR *aDescription)
{
__try {
  *aDescription = NULL;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsAutoString descr;
  nsresult rv = tableAcc->GetColumnDescription (aColumn, descr);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (!::SysReAllocStringLen(aDescription, descr.get(), descr.Length()))
    return E_OUTOFMEMORY;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}

STDMETHODIMP
CAccessibleTable::get_columnExtentAt(long aRow, long aColumn,
                                     long *nColumnsSpanned)
{
__try {
  *nColumnsSpanned = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRInt32 columnsSpanned = 0;
  nsresult rv = tableAcc->GetColumnExtentAt(aRow, aColumn, &columnsSpanned);
  *nColumnsSpanned = columnsSpanned;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_columnHeader(IAccessibleTable **aAccessibleTable,
                                   long *aStartingRowIndex)
{
__try {
  *aAccessibleTable = NULL;

  
  *aStartingRowIndex = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsCOMPtr<nsIAccessibleTable> header;
  nsresult rv = tableAcc->GetColumnHeader(getter_AddRefs(header));
  if (NS_FAILED(rv))
    return E_FAIL;

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(header));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  rv = winAccessNode->QueryNativeInterface(IID_IAccessibleTable, &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aAccessibleTable = static_cast<IAccessibleTable*>(instancePtr);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleTable::get_columnIndex(long aChildIndex, long *aColumnIndex)
{
__try {
  *aColumnIndex = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRInt32 columnIndex = 0;
  nsresult rv = tableAcc->GetColumnAtIndex(aChildIndex, &columnIndex);
  *aColumnIndex = columnIndex;
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_nColumns(long *aColumnCount)
{
__try {
  *aColumnCount = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRInt32 columnCount = 0;
  nsresult rv = tableAcc->GetColumns(&columnCount);
  *aColumnCount = columnCount;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_nRows(long *aRowCount)
{
__try {
  *aRowCount = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRInt32 rowCount = 0;
  nsresult rv = tableAcc->GetRows(&rowCount);
  *aRowCount = rowCount;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_nSelectedChildren(long *aChildCount)
{
__try {
  *aChildCount = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRUint32 count = 0;
  nsresult rv = tableAcc->GetSelectedCellsCount(&count);
  *aChildCount = count;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_nSelectedColumns(long *aColumnCount)
{
__try {
  *aColumnCount = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRUint32 count = 0;
  nsresult rv = tableAcc->GetSelectedColumnsCount(&count);
  *aColumnCount = count;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_nSelectedRows(long *aRowCount)
{
__try {
  *aRowCount = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRUint32 count = 0;
  nsresult rv = tableAcc->GetSelectedRowsCount(&count);
  *aRowCount = count;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_rowDescription(long aRow, BSTR *aDescription)
{
__try {
  *aDescription = NULL;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsAutoString descr;
  nsresult rv = tableAcc->GetRowDescription (aRow, descr);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (!::SysReAllocStringLen(aDescription, descr.get(), descr.Length()))
    return E_OUTOFMEMORY;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}

STDMETHODIMP
CAccessibleTable::get_rowExtentAt(long aRow, long aColumn, long *aNRowsSpanned)
{
__try {
  *aNRowsSpanned = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRInt32 rowsSpanned = 0;
  nsresult rv = tableAcc->GetRowExtentAt(aRow, aColumn, &rowsSpanned);
  *aNRowsSpanned = rowsSpanned;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_rowHeader(IAccessibleTable **aAccessibleTable,
                                long *aStartingColumnIndex)
{
__try {
  *aAccessibleTable = NULL;

  
  *aStartingColumnIndex = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsCOMPtr<nsIAccessibleTable> header;
  nsresult rv = tableAcc->GetRowHeader(getter_AddRefs(header));
  if (NS_FAILED(rv))
    return E_FAIL;

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(header));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  rv = winAccessNode->QueryNativeInterface(IID_IAccessibleTable,
                                           &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aAccessibleTable = static_cast<IAccessibleTable*>(instancePtr);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleTable::get_rowIndex(long aChildIndex, long *aRowIndex)
{
__try {
  *aRowIndex = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRInt32 rowIndex = 0;
  nsresult rv = tableAcc->GetRowAtIndex(aChildIndex, &rowIndex);
  *aRowIndex = rowIndex;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_selectedChildren(long aMaxChildren, long **aChildren,
                                       long *aNChildren)
{
__try {
  return GetSelectedItems(aMaxChildren, aChildren, aNChildren, ITEMSTYPE_CELLS);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_selectedColumns(long aMaxColumns, long **aColumns,
                                      long *aNColumns)
{
__try {
  return GetSelectedItems(aMaxColumns, aColumns, aNColumns, ITEMSTYPE_COLUMNS);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_selectedRows(long aMaxRows, long **aRows, long *aNRows)
{
__try {
  return GetSelectedItems(aMaxRows, aRows, aNRows, ITEMSTYPE_ROWS);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_summary(IUnknown **aAccessible)
{
  *aAccessible = NULL;

  
  
  
  
  return S_OK;
}

STDMETHODIMP
CAccessibleTable::get_isColumnSelected(long aColumn, boolean *aIsSelected)
{
__try {
  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRBool isSelected = PR_FALSE;
  nsresult rv = tableAcc->IsColumnSelected(aColumn, &isSelected);
  *aIsSelected = isSelected;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_isRowSelected(long aRow, boolean *aIsSelected)
{
__try {
  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRBool isSelected = PR_FALSE;
  nsresult rv = tableAcc->IsRowSelected(aRow, &isSelected);
  *aIsSelected = isSelected;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_isSelected(long aRow, long aColumn, boolean *aIsSelected)
{
__try {
  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRBool isSelected = PR_FALSE;
  nsresult rv = tableAcc->IsCellSelected(aRow, aColumn, &isSelected);
  *aIsSelected = isSelected;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::selectRow(long aRow)
{
__try {
  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsresult rv = tableAcc->SelectRow(aRow);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::selectColumn(long aColumn)
{
__try {
  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsresult rv = tableAcc->SelectColumn(aColumn);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::unselectRow(long aRow)
{
__try {
  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsresult rv = tableAcc->UnselectRow(aRow);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::unselectColumn(long aColumn)
{
__try {
  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  nsresult rv = tableAcc->UnselectColumn(aColumn);
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleTable::get_rowColumnExtentsAtIndex(long aIndex, long *aRow,
                                              long *aColumn,
                                              long *aRowExtents,
                                              long *aColumnExtents,
                                              boolean *aIsSelected)
{
__try {
  *aRow = 0;
  *aColumn = 0;
  *aRowExtents = 0;
  *aColumnExtents = 0;
  *aIsSelected = false;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRInt32 row = -1;
  nsresult rv = tableAcc->GetRowAtIndex(aIndex, &row);
  if (NS_FAILED(rv))
    return E_FAIL;

  PRInt32 column = -1;
  rv = tableAcc->GetColumnAtIndex(aIndex, &column);
  if (NS_FAILED(rv))
    return E_FAIL;

  PRInt32 rowExtents = 0;
  rv = tableAcc->GetRowExtentAt(row, column, &rowExtents);
  if (NS_FAILED(rv))
    return E_FAIL;

  PRInt32 columnExtents = 0;
  rv = tableAcc->GetColumnExtentAt(row, column, &columnExtents);
  if (NS_FAILED(rv))
    return E_FAIL;

  PRBool isSelected = PR_FALSE;
  rv = tableAcc->IsCellSelected(row, column, &isSelected);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aRow = row;
  *aColumn = column;
  *aRowExtents = rowExtents;
  *aColumnExtents = columnExtents;
  *aIsSelected = isSelected;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleTable::get_modelChange(IA2TableModelChange *aModelChange)
{
  aModelChange = NULL;
  return E_NOTIMPL;
}



HRESULT
CAccessibleTable::GetSelectedItems(long aMaxItems, long **aItems,
                                   long *aItemsCount, eItemsType aType)
{
  *aItemsCount = 0;

  nsCOMPtr<nsIAccessibleTable> tableAcc(do_QueryInterface(this));
  NS_ASSERTION(tableAcc, CANT_QUERY_ASSERTION_MSG);
  if (!tableAcc)
    return E_FAIL;

  PRUint32 size = 0;
  PRInt32 *items = NULL;

  nsresult rv = NS_OK;
  switch (aType) {
    case ITEMSTYPE_CELLS:
      rv = tableAcc->GetSelectedCells(&size, &items);
      break;
    case ITEMSTYPE_COLUMNS:
      rv = tableAcc->GetSelectedColumns(&size, &items);
      break;
    case ITEMSTYPE_ROWS:
      rv = tableAcc->GetSelectedRows(&size, &items);
      break;
    default:
      return E_FAIL;
  }

  if (NS_FAILED(rv))
    return E_FAIL;

  if (size == 0 || !items)
    return S_OK;

  PRUint32 maxSize = size < (PRUint32)aMaxItems ? size : aMaxItems;
  *aItemsCount = maxSize;

  for (PRUint32 index = 0; index < maxSize; ++index)
    (*aItems)[index] = items[index];

  nsMemory::Free(items);
  return S_OK;
}

