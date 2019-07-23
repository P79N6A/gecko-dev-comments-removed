





































#include "nsXULTreeAccessibleWrap.h"
#include "nsTextFormatter.h"
#include "nsIFrame.h"





nsXULTreeAccessibleWrap::nsXULTreeAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell):
nsXULTreeAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsXULTreeAccessibleWrap::GetRole(PRUint32 *aRole)
{
  NS_ASSERTION(mTree, "No tree view");

  nsCOMPtr<nsITreeColumns> cols;
  mTree->GetColumns(getter_AddRefs(cols));
  nsCOMPtr<nsITreeColumn> primaryCol;
  if (cols) {
    cols->GetPrimaryColumn(getter_AddRefs(primaryCol));
  }
  
  
  *aRole = primaryCol ? nsIAccessibleRole::ROLE_OUTLINE :
                        nsIAccessibleRole::ROLE_LIST;

  return NS_OK;
}





nsXULTreeitemAccessibleWrap::nsXULTreeitemAccessibleWrap(nsIAccessible *aParent, 
                                                         nsIDOMNode *aDOMNode, 
                                                         nsIWeakReference *aShell, 
                                                         PRInt32 aRow, 
                                                         nsITreeColumn* aColumn) :
nsXULTreeitemAccessible(aParent, aDOMNode, aShell, aRow, aColumn)
{
}

NS_IMETHODIMP nsXULTreeitemAccessibleWrap::GetRole(PRUint32 *aRole)
{
  
  
  NS_ASSERTION(mColumn, "mColumn is null");
  PRBool isPrimary = PR_FALSE;
  mColumn->GetPrimary(&isPrimary);
  *aRole = isPrimary ? nsIAccessibleRole::ROLE_OUTLINEITEM :
                       nsIAccessibleRole::ROLE_LISTITEM;
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeitemAccessibleWrap::GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  nsresult rv = nsXULTreeitemAccessible::GetBounds(x, y, width, height);
  if (NS_FAILED(rv)) {
    return rv;
  }
  nsIFrame *frame = GetFrame();
  if (frame) {
    
    PRInt32 cellStartX, cellStartY;
    mTree->GetCoordsForCellItem(mRow, mColumn, EmptyCString(), &cellStartX, &cellStartY, width, height);
    
    *width = GetPresContext()->AppUnitsToDevPixels(frame->GetRect().width) -
             cellStartX;
  }
  return NS_OK;
}

NS_IMETHODIMP nsXULTreeitemAccessibleWrap::GetName(nsAString& aName)
{
  nsCOMPtr<nsITreeColumns> cols;
  mTree->GetColumns(getter_AddRefs(cols));
  if (!cols) {
    return NS_OK;
  }
  nsCOMPtr<nsITreeColumn> column;
  cols->GetFirstColumn(getter_AddRefs(column));
  while (column) {
    nsAutoString colText;
    mTreeView->GetCellText(mRow, column, colText);
    aName += colText + NS_LITERAL_STRING("  ");
    nsCOMPtr<nsITreeColumn> nextColumn;
    column->GetNext(getter_AddRefs(nextColumn));
    column = nextColumn;
  }
  return NS_OK;
}

