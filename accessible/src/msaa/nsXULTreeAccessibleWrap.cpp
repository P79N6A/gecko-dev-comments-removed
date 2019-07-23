





































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

NS_IMETHODIMP nsXULTreeitemAccessibleWrap::GetDescription(nsAString& aDescription)
{
  if (!mParent || !mWeakShell || !mTreeView) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 itemRole;
  GetRole(&itemRole);
  if (itemRole == nsIAccessibleRole::ROLE_LISTITEM) {
    return nsAccessibleWrap::GetDescription(aDescription);
  }

  aDescription.Truncate();

  PRInt32 level;
  if (NS_FAILED(mTreeView->GetLevel(mRow, &level))) {
    return NS_OK;
  }

  PRInt32 testRow = -1;
  if (level > 0) {
    mTreeView->GetParentIndex(mRow, &testRow);
  }

  PRInt32 numRows;
  mTreeView->GetRowCount(&numRows);

  PRInt32 indexInParent = 0, numSiblings = 0;

  
  while (++ testRow < numRows) {
    PRInt32 testLevel = 0;
    mTreeView->GetLevel(testRow, &testLevel);
    if (testLevel == level) {
      if (testRow <= mRow) {
        ++indexInParent;
      }
      ++numSiblings;
    }
    else if (testLevel < level) {
      break;
    }
  }

  
  testRow = mRow;
  PRInt32 numChildren = 0;
  while (++ testRow < numRows) {
    PRInt32 testLevel = 0;
    mTreeView->GetLevel(testRow, &testLevel);
    if (testLevel <= level) {
      break;
    }
    else if (testLevel == level + 1) {
      ++ numChildren;
    }
  }

  
  
  nsTextFormatter::ssprintf(aDescription, NS_LITERAL_STRING("L%d, %d of %d with %d").get(),
                            level + 1, indexInParent, numSiblings, numChildren);

  return NS_OK;
}

