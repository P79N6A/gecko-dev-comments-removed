





































#include "nsXULTreeAccessibleWrap.h"

#include "nsIBoxObject.h"
#include "nsTextFormatter.h"
#include "nsIFrame.h"





nsXULTreeAccessibleWrap::nsXULTreeAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell):
nsXULTreeAccessible(aDOMNode, aShell)
{
}

nsresult
nsXULTreeAccessibleWrap::GetRoleInternal(PRUint32 *aRole)
{
  NS_ENSURE_STATE(mTree);

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

nsresult
nsXULTreeitemAccessibleWrap::GetRoleInternal(PRUint32 *aRole)
{
  
  
  NS_ENSURE_STATE(mColumn);
  PRBool isPrimary = PR_FALSE;
  mColumn->GetPrimary(&isPrimary);
  *aRole = isPrimary ? nsIAccessibleRole::ROLE_OUTLINEITEM :
                       nsIAccessibleRole::ROLE_LISTITEM;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeitemAccessibleWrap::GetBounds(PRInt32 *aX, PRInt32 *aY,
                                       PRInt32 *aWidth, PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER(aX);
  *aX = 0;
  NS_ENSURE_ARG_POINTER(aY);
  *aY = 0;
  NS_ENSURE_ARG_POINTER(aWidth);
  *aWidth = 0;
  NS_ENSURE_ARG_POINTER(aHeight);
  *aHeight = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsIDOMElement> tcElm;
  mTree->GetTreeBody(getter_AddRefs(tcElm));
  nsCOMPtr<nsIDOMXULElement> tcXULElm(do_QueryInterface(tcElm));
  NS_ENSURE_STATE(tcXULElm);

  nsCOMPtr<nsIBoxObject> boxObj;
  tcXULElm->GetBoxObject(getter_AddRefs(boxObj));
  NS_ENSURE_STATE(boxObj);

  PRInt32 cellStartX, cellWidth;
  nsresult rv = mTree->GetCoordsForCellItem(mRow, mColumn, EmptyCString(),
                                            &cellStartX, aY,
                                            &cellWidth, aHeight);
  NS_ENSURE_SUCCESS(rv, rv);

  boxObj->GetWidth(aWidth);

  PRInt32 tcX = 0, tcY = 0;
  boxObj->GetScreenX(&tcX);
  boxObj->GetScreenY(&tcY);

  *aX = tcX;
  *aY += tcY;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeitemAccessibleWrap::GetName(nsAString& aName)
{
  NS_ENSURE_STATE(mTree);
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

