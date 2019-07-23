






































#include "nsIDOMElement.h"
#include "nsITreeSelection.h"
#include "nsITreeColumns.h"
#include "nsXULTreeAccessibleWrap.h"





nsXULTreeGridAccessibleWrap::
  nsXULTreeGridAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell):
  nsXULTreeGridAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP
nsXULTreeGridAccessibleWrap::GetColumnHeader(nsIAccessibleTable **aColumnHeader)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIAccessible> acc;
  nsAccessible::GetFirstChild(getter_AddRefs(acc));
  NS_ENSURE_TRUE(acc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAccessibleTable> accTable(do_QueryInterface(acc, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  *aColumnHeader = accTable;
  NS_IF_ADDREF(*aColumnHeader);

  return rv;
}

NS_IMETHODIMP
nsXULTreeGridAccessibleWrap::GetColumnDescription(PRInt32 aColumn, nsAString & _retval)
{
  nsCOMPtr<nsIAccessibleTable> columnHeader;
  nsresult rv = GetColumnHeader(getter_AddRefs(columnHeader));
  if (NS_SUCCEEDED(rv) && columnHeader) {
    return columnHeader->GetColumnDescription(aColumn, _retval);
  }
  return NS_ERROR_FAILURE;
}




NS_IMPL_ISUPPORTS_INHERITED1(nsXULTreeColumnsAccessibleWrap, nsXULTreeColumnsAccessible, nsIAccessibleTable)

nsXULTreeColumnsAccessibleWrap::nsXULTreeColumnsAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell):
nsXULTreeColumnsAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetCaption(nsIAccessible **aCaption)
{
  *aCaption = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetSummary(nsAString &aSummary)
{
  aSummary.Truncate();
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetColumns(PRInt32 *aColumns)
{
  nsresult rv = GetChildCount(aColumns);
  return *aColumns > 0 ? rv : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetColumnHeader(nsIAccessibleTable * *aColumnHeader)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetRows(PRInt32 *aRows)
{
  NS_ENSURE_ARG_POINTER(aRows);

  *aRows = 1;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetRowHeader(nsIAccessibleTable * *aRowHeader)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULTreeColumnsAccessibleWrap::GetSelectedCellsCount(PRUint32* aCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULTreeColumnsAccessibleWrap::GetSelectedColumnsCount(PRUint32* aCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULTreeColumnsAccessibleWrap::GetSelectedRowsCount(PRUint32* aCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULTreeColumnsAccessibleWrap::GetSelectedCells(PRUint32 *aNumCells,
                                                 PRInt32 **aCells)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetSelectedColumns(PRUint32 *columnsSize, PRInt32 **columns)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetSelectedRows(PRUint32 *rowsSize, PRInt32 **rows)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::CellRefAt(PRInt32 aRow, PRInt32 aColumn, nsIAccessible **_retval)
{
  nsCOMPtr<nsIAccessible> next, temp;
  GetFirstChild(getter_AddRefs(next));
  NS_ENSURE_TRUE(next, NS_ERROR_FAILURE);

  for (PRInt32 col = 0; col < aColumn; col++) {
    next->GetNextSibling(getter_AddRefs(temp));
    NS_ENSURE_TRUE(temp, NS_ERROR_FAILURE);

    next = temp;
  }

  *_retval = next;
  NS_IF_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetIndexAt(PRInt32 aRow, PRInt32 aColumn, PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = aColumn;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetColumnAtIndex(PRInt32 aIndex, PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = aIndex;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetRowAtIndex(PRInt32 aIndex, PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = 0;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetColumnExtentAt(PRInt32 aRow, PRInt32 aColumn, PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = 1;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetRowExtentAt(PRInt32 aRow, PRInt32 aColumn, PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = 1;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetColumnDescription(PRInt32 aColumn, nsAString & _retval)
{
  nsCOMPtr<nsIAccessible> column;  
  nsresult rv = CellRefAt(0, aColumn, getter_AddRefs(column));
  if (NS_SUCCEEDED(rv) && column) {
    return column->GetName(_retval);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::GetRowDescription(PRInt32 aRow, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::IsColumnSelected(PRInt32 aColumn, PRBool *_retval)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::IsRowSelected(PRInt32 aRow, PRBool *_retval)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::IsCellSelected(PRInt32 aRow, PRInt32 aColumn, PRBool *_retval)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULTreeColumnsAccessibleWrap::SelectRow(PRInt32 aRow)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULTreeColumnsAccessibleWrap::SelectColumn(PRInt32 aColumn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULTreeColumnsAccessibleWrap::UnselectRow(PRInt32 aRow)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXULTreeColumnsAccessibleWrap::UnselectColumn(PRInt32 aColumn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULTreeColumnsAccessibleWrap::IsProbablyForLayout(PRBool *aIsProbablyForLayout)
{
  *aIsProbablyForLayout = PR_FALSE;
  return NS_OK;
}
