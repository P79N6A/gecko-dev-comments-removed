





































#include "nsXULTreeGridAccessibleWrap.h"

#include "nsAccCache.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsEventShell.h"

#include "nsITreeSelection.h"





nsXULTreeGridAccessible::
  nsXULTreeGridAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell) :
  nsXULTreeAccessible(aDOMNode, aShell)
{
}




NS_IMPL_ISUPPORTS_INHERITED1(nsXULTreeGridAccessible,
                             nsXULTreeAccessible,
                             nsIAccessibleTable)




NS_IMETHODIMP
nsXULTreeGridAccessible::GetCaption(nsIAccessible **aCaption)
{
  NS_ENSURE_ARG_POINTER(aCaption);
  *aCaption = nsnull;

  return IsDefunct() ? NS_ERROR_FAILURE : NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetSummary(nsAString &aSummary)
{
  aSummary.Truncate();
  return IsDefunct() ? NS_ERROR_FAILURE : NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetColumnCount(PRInt32 *aColumnCount)
{
  NS_ENSURE_ARG_POINTER(aColumnCount);
  *aColumnCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aColumnCount = nsCoreUtils::GetSensibleColumnCount(mTree);
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetRowCount(PRInt32 *arowCount)
{
  NS_ENSURE_ARG_POINTER(arowCount);
  *arowCount = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  return mTreeView->GetRowCount(arowCount);
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetSelectedCellCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  PRUint32 selectedrowCount = 0;
  nsresult rv = GetSelectedRowCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 columnCount = 0;
  rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aCount = selectedrowCount * columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetSelectedColumnCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  

  PRInt32 rowCount = 0;
  nsresult rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 selectedrowCount = 0;
  rv = GetSelectionCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowCount == selectedrowCount) {
    PRInt32 columnCount = 0;
    rv = GetColumnCount(&columnCount);
    NS_ENSURE_SUCCESS(rv, rv);

    *aCount = columnCount;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetSelectedRowCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 selectedrowCount = 0;
  nsresult rv = GetSelectionCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aCount = selectedrowCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetSelectedCells(nsIArray **aCells)
{
  NS_ENSURE_ARG_POINTER(aCells);
  *aCells = nsnull;

  nsCOMPtr<nsIMutableArray> selCells = do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_TRUE(selCells, NS_ERROR_FAILURE);

  PRInt32 selectedrowCount = 0;
  nsresult rv = GetSelectionCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 columnCount = 0;
  rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsITreeSelection> selection;
  rv = mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rowCount = 0;
  rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSelected;
  for (PRInt32 rowIdx = 0; rowIdx < rowCount; rowIdx++) {
    selection->IsSelected(rowIdx, &isSelected);
    if (isSelected) {
      for (PRInt32 colIdx = 0; colIdx < columnCount; colIdx++) {
        nsCOMPtr<nsIAccessible> cell;
        GetCellAt(rowIdx, colIdx, getter_AddRefs(cell));
        selCells->AppendElement(cell, PR_FALSE);
      }
    }
  }

  NS_ADDREF(*aCells = selCells);
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetSelectedCellIndices(PRUint32 *aCellsCount,
                                                PRInt32 **aCells)
{
  NS_ENSURE_ARG_POINTER(aCellsCount);
  *aCellsCount = 0;
  NS_ENSURE_ARG_POINTER(aCells);
  *aCells = nsnull;

  PRInt32 selectedrowCount = 0;
  nsresult rv = GetSelectionCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 columnCount = 0;
  rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 selectedCellCount = selectedrowCount * columnCount;
  PRInt32* outArray = static_cast<PRInt32*>(
    nsMemory::Alloc(selectedCellCount * sizeof(PRInt32)));
  NS_ENSURE_TRUE(outArray, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsITreeSelection> selection;
  rv = mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rowCount = 0;
  rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSelected;
  for (PRInt32 rowIdx = 0, arrayIdx = 0; rowIdx < rowCount; rowIdx++) {
    selection->IsSelected(rowIdx, &isSelected);
    if (isSelected) {
      for (PRInt32 colIdx = 0; colIdx < columnCount; colIdx++)
        outArray[arrayIdx++] = rowIdx * columnCount + colIdx;
    }
  }

  *aCellsCount = selectedCellCount;
  *aCells = outArray;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetSelectedColumnIndices(PRUint32 *acolumnCount,
                                                  PRInt32 **aColumns)
{
  NS_ENSURE_ARG_POINTER(acolumnCount);
  *acolumnCount = 0;
  NS_ENSURE_ARG_POINTER(aColumns);
  *aColumns = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  

  PRInt32 rowCount = 0;
  nsresult rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 selectedrowCount = 0;
  rv = GetSelectionCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowCount != selectedrowCount)
    return NS_OK;

  PRInt32 columnCount = 0;
  rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32* outArray = static_cast<PRInt32*>(
    nsMemory::Alloc(columnCount * sizeof(PRInt32)));
  NS_ENSURE_TRUE(outArray, NS_ERROR_OUT_OF_MEMORY);

  for (PRInt32 colIdx = 0; colIdx < columnCount; colIdx++)
    outArray[colIdx] = colIdx;

  *acolumnCount = columnCount;
  *aColumns = outArray;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetSelectedRowIndices(PRUint32 *arowCount,
                                               PRInt32 **aRows)
{
  NS_ENSURE_ARG_POINTER(arowCount);
  *arowCount = 0;
  NS_ENSURE_ARG_POINTER(aRows);
  *aRows = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 selectedrowCount = 0;
  nsresult rv = GetSelectionCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32* outArray = static_cast<PRInt32*>(
    nsMemory::Alloc(selectedrowCount * sizeof(PRInt32)));
  NS_ENSURE_TRUE(outArray, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsITreeSelection> selection;
  rv = mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rowCount = 0;
  rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSelected;
  for (PRInt32 rowIdx = 0, arrayIdx = 0; rowIdx < rowCount; rowIdx++) {
    selection->IsSelected(rowIdx, &isSelected);
    if (isSelected)
      outArray[arrayIdx++] = rowIdx;
  }

  *arowCount = selectedrowCount;
  *aRows = outArray;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetCellAt(PRInt32 aRowIndex, PRInt32 aColumnIndex,
                                   nsIAccessible **aCell)
{
  NS_ENSURE_ARG_POINTER(aCell);
  *aCell = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAccessible *rowAccessible = GetTreeItemAccessible(aRowIndex);
  if (!rowAccessible)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsITreeColumn> column =
  nsCoreUtils::GetSensibleColumnAt(mTree, aColumnIndex);
  if (!column)
    return NS_ERROR_INVALID_ARG;

  nsRefPtr<nsXULTreeItemAccessibleBase> rowAcc =
    nsAccUtils::QueryObject<nsXULTreeItemAccessibleBase>(rowAccessible);

  NS_IF_ADDREF(*aCell = rowAcc->GetCellAccessible(column));
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetCellIndexAt(PRInt32 aRowIndex, PRInt32 aColumnIndex,
                                        PRInt32 *aCellIndex)
{
  NS_ENSURE_ARG_POINTER(aCellIndex);
  *aCellIndex = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 columnCount = 0;
  nsresult rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aCellIndex = aRowIndex * columnCount + aColumnIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetColumnIndexAt(PRInt32 aCellIndex,
                                          PRInt32 *aColumnIndex)
{
  NS_ENSURE_ARG_POINTER(aColumnIndex);
  *aColumnIndex = -1;

  PRInt32 columnCount = 0;
  nsresult rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aColumnIndex = aCellIndex % columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetRowIndexAt(PRInt32 aCellIndex, PRInt32 *aRowIndex)
{
  NS_ENSURE_ARG_POINTER(aRowIndex);
  *aRowIndex = -1;

  PRInt32 columnCount;
  nsresult rv = GetColumnCount(&columnCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRowIndex = aCellIndex / columnCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetColumnExtentAt(PRInt32 aRowIndex,
                                           PRInt32 aColumnIndex,
                                           PRInt32 *aExtentCount)
{
  NS_ENSURE_ARG_POINTER(aExtentCount);
  *aExtentCount = 1;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetRowExtentAt(PRInt32 aRowIndex, PRInt32 aColumnIndex,
                                        PRInt32 *aExtentCount)
{
  NS_ENSURE_ARG_POINTER(aExtentCount);
  *aExtentCount = 1;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetColumnDescription(PRInt32 aColumnIndex,
                                              nsAString& aDescription)
{
  aDescription.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIAccessible> treeColumns;
  nsAccessible::GetFirstChild(getter_AddRefs(treeColumns));
  if (treeColumns) {
    nsCOMPtr<nsIAccessible> treeColumnItem;
    treeColumns->GetChildAt(aColumnIndex, getter_AddRefs(treeColumnItem));
    if (treeColumnItem)
      return treeColumnItem->GetName(aDescription);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::GetRowDescription(PRInt32 aRowIndex,
                                           nsAString& aDescription)
{
  aDescription.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::IsColumnSelected(PRInt32 aColumnIndex,
                                          PRBool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = PR_FALSE;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  
  
  PRInt32 rowCount = 0;
  nsresult rv = GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 selectedrowCount = 0;
  rv = GetSelectionCount(&selectedrowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  *aIsSelected = rowCount == selectedrowCount;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::IsRowSelected(PRInt32 aRowIndex, PRBool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = PR_FALSE;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  nsresult rv = mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return selection->IsSelected(aRowIndex, aIsSelected);
}

NS_IMETHODIMP
nsXULTreeGridAccessible::IsCellSelected(PRInt32 aRowIndex, PRInt32 aColumnIndex,
                                        PRBool *aIsSelected)
{
  return IsRowSelected(aRowIndex, aIsSelected);
}

NS_IMETHODIMP
nsXULTreeGridAccessible::SelectRow(PRInt32 aRowIndex)
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_STATE(selection);

  return selection->Select(aRowIndex);
}

NS_IMETHODIMP
nsXULTreeGridAccessible::SelectColumn(PRInt32 aColumnIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::UnselectRow(PRInt32 aRowIndex)
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_STATE(selection);

  return selection->ClearRange(aRowIndex, aRowIndex);
}

NS_IMETHODIMP
nsXULTreeGridAccessible::UnselectColumn(PRInt32 aColumnIndex)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridAccessible::IsProbablyForLayout(PRBool *aIsProbablyForLayout)
{
  NS_ENSURE_ARG_POINTER(aIsProbablyForLayout);
  *aIsProbablyForLayout = PR_FALSE;

  return NS_OK;
}




nsresult
nsXULTreeGridAccessible::GetRoleInternal(PRUint32 *aRole)
{
  nsCOMPtr<nsITreeColumns> treeColumns;
  mTree->GetColumns(getter_AddRefs(treeColumns));
  NS_ENSURE_STATE(treeColumns);

  nsCOMPtr<nsITreeColumn> primaryColumn;
  treeColumns->GetPrimaryColumn(getter_AddRefs(primaryColumn));

  *aRole = primaryColumn ?
    nsIAccessibleRole::ROLE_TREE_TABLE :
    nsIAccessibleRole::ROLE_TABLE;

  return NS_OK;
}




already_AddRefed<nsAccessible>
nsXULTreeGridAccessible::CreateTreeItemAccessible(PRInt32 aRow)
{
  nsRefPtr<nsAccessible> accessible =
    new nsXULTreeGridRowAccessible(mDOMNode, mWeakShell, this, mTree,
                                   mTreeView, aRow);

  return accessible.forget();
}






nsXULTreeGridRowAccessible::
  nsXULTreeGridRowAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                             nsAccessible *aTreeAcc, nsITreeBoxObject* aTree,
                             nsITreeView *aTreeView, PRInt32 aRow) :
  nsXULTreeItemAccessibleBase(aDOMNode, aShell, aTreeAcc, aTree, aTreeView, aRow)
{
  mAccessibleCache.Init(kDefaultTreeCacheSize);
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULTreeGridRowAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXULTreeGridRowAccessible,
                                                  nsAccessible)
CycleCollectorTraverseCache(tmp->mAccessibleCache, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXULTreeGridRowAccessible,
                                                nsAccessible)
ClearCache(tmp->mAccessibleCache);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXULTreeGridRowAccessible)
NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsXULTreeGridRowAccessible)
NS_INTERFACE_MAP_END_INHERITING(nsXULTreeItemAccessibleBase)

NS_IMPL_ADDREF_INHERITED(nsXULTreeGridRowAccessible,
                         nsXULTreeItemAccessibleBase)
NS_IMPL_RELEASE_INHERITED(nsXULTreeGridRowAccessible,
                          nsXULTreeItemAccessibleBase)




nsresult
nsXULTreeGridRowAccessible::Shutdown()
{
  ClearCache(mAccessibleCache);
  return nsXULTreeItemAccessibleBase::Shutdown();
}




nsresult
nsXULTreeGridRowAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_ROW;
  return NS_OK;
}

nsresult
nsXULTreeGridRowAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                            PRBool aDeepestChild,
                                            nsIAccessible **aChild)
{
  nsIFrame *frame = GetFrame();
  if (!frame)
    return NS_ERROR_FAILURE;

  nsPresContext *presContext = frame->PresContext();
  nsCOMPtr<nsIPresShell> presShell = presContext->PresShell();

  nsIFrame *rootFrame = presShell->GetRootFrame();
  NS_ENSURE_STATE(rootFrame);

  nsIntRect rootRect = rootFrame->GetScreenRectExternal();

  PRInt32 clientX = presContext->DevPixelsToIntCSSPixels(aX - rootRect.x);
  PRInt32 clientY = presContext->DevPixelsToIntCSSPixels(aY - rootRect.y);

  PRInt32 row = -1;
  nsCOMPtr<nsITreeColumn> column;
  nsCAutoString childEltUnused;
  mTree->GetCellAt(clientX, clientY, &row, getter_AddRefs(column),
                   childEltUnused);

  
  if (row != mRow || !column)
    return NS_OK;

  NS_IF_ADDREF(*aChild = GetCellAccessible(column));
  return NS_OK;
}

nsAccessible*
nsXULTreeGridRowAccessible::GetChildAt(PRUint32 aIndex)
{
  if (IsDefunct())
    return nsnull;

  nsCOMPtr<nsITreeColumn> column =
    nsCoreUtils::GetSensibleColumnAt(mTree, aIndex);
  if (!column)
    return nsnull;

  return GetCellAccessible(column);
}

PRInt32
nsXULTreeGridRowAccessible::GetChildCount()
{
  if (IsDefunct())
    return -1;

  return nsCoreUtils::GetSensibleColumnCount(mTree);
}

PRInt32
nsXULTreeGridRowAccessible::GetIndexOf(nsIAccessible *aChild)
{
  if (IsDefunct())
    return -1;

  nsRefPtr<nsXULTreeGridCellAccessible> cell =
    nsAccUtils::QueryObject<nsXULTreeGridCellAccessible>(aChild);

  return cell ? cell->GetColumnIndex() : -1;
}




nsAccessible*
nsXULTreeGridRowAccessible::GetCellAccessible(nsITreeColumn* aColumn)
{
  NS_PRECONDITION(aColumn, "No tree column!");

  void* key = static_cast<void*>(aColumn);
  nsRefPtr<nsAccessible> accessible = mAccessibleCache.GetWeak(key);

  if (!accessible) {
    accessible =
      new nsXULTreeGridCellAccessibleWrap(mDOMNode, mWeakShell, this, mTree,
                                          mTreeView, mRow, aColumn);
    if (!accessible)
      return nsnull;

    nsresult rv = accessible->Init();
    if (NS_FAILED(rv)) {
      accessible->Shutdown();
      return nsnull;
    }

    if (!mAccessibleCache.Put(key, accessible))
      return nsnull;
  }

  return accessible;
}

void
nsXULTreeGridRowAccessible::RowInvalidated(PRInt32 aStartColIdx,
                                           PRInt32 aEndColIdx)
{
  nsCOMPtr<nsITreeColumns> treeColumns;
  mTree->GetColumns(getter_AddRefs(treeColumns));
  if (!treeColumns)
    return;

  for (PRInt32 colIdx = aStartColIdx; colIdx <= aEndColIdx; ++colIdx) {
    nsCOMPtr<nsITreeColumn> column;
    treeColumns->GetColumnAt(colIdx, getter_AddRefs(column));
    if (column && !nsCoreUtils::IsColumnHidden(column)) {
      nsAccessible *cellAccessible = GetCellAccessible(column);
      if (cellAccessible) {
        nsRefPtr<nsXULTreeGridCellAccessible> cellAcc =
          nsAccUtils::QueryObject<nsXULTreeGridCellAccessible>(cellAccessible);

        cellAcc->CellInvalidated();
      }
    }
  }
}




void
nsXULTreeGridRowAccessible::CacheChildren()
{
}





nsXULTreeGridCellAccessible::
nsXULTreeGridCellAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                            nsXULTreeGridRowAccessible *aRowAcc,
                            nsITreeBoxObject *aTree, nsITreeView *aTreeView,
                            PRInt32 aRow, nsITreeColumn* aColumn) :
  nsLeafAccessible(aDOMNode, aShell), mTree(aTree),
  mTreeView(aTreeView), mRow(aRow), mColumn(aColumn)
{
  mParent = aRowAcc;
}




NS_IMPL_ISUPPORTS_INHERITED2(nsXULTreeGridCellAccessible,
                             nsLeafAccessible,
                             nsIAccessibleTableCell,
                             nsXULTreeGridCellAccessible)




NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetUniqueID(void **aUniqueID)
{
  NS_ENSURE_ARG_POINTER(aUniqueID);
  *aUniqueID = static_cast<void*>(this);

  return NS_OK;
}




NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetFocusedChild(nsIAccessible **aFocusedChild) 
{
  NS_ENSURE_ARG_POINTER(aFocusedChild);
  *aFocusedChild = nsnull;

  return IsDefunct() ? NS_ERROR_FAILURE : NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetName(nsAString& aName)
{
  aName.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  mTreeView->GetCellText(mRow, mColumn, aName);

  
  
  
  
  
  if (aName.IsEmpty())
    mTreeView->GetCellValue(mRow, mColumn, aName);

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetBounds(PRInt32 *aX, PRInt32 *aY,
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

  
  
  nsCOMPtr<nsIBoxObject> boxObj = nsCoreUtils::GetTreeBodyBoxObject(mTree);
  NS_ENSURE_STATE(boxObj);

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = mTree->GetCoordsForCellItem(mRow, mColumn,
                                            NS_LITERAL_CSTRING("cell"),
                                            &x, &y, &width, &height);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 tcX = 0, tcY = 0;
  boxObj->GetScreenX(&tcX);
  boxObj->GetScreenY(&tcY);
  x += tcX;
  y += tcY;

  nsPresContext *presContext = GetPresContext();
  *aX = presContext->CSSPixelsToDevPixels(x);
  *aY = presContext->CSSPixelsToDevPixels(y);
  *aWidth = presContext->CSSPixelsToDevPixels(width);
  *aHeight = presContext->CSSPixelsToDevPixels(height);

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetNumActions(PRUint8 *aActionsCount)
{
  NS_ENSURE_ARG_POINTER(aActionsCount);
  *aActionsCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRBool isCycler = PR_FALSE;
  mColumn->GetCycler(&isCycler);
  if (isCycler) {
    *aActionsCount = 1;
    return NS_OK;
  }

  PRInt16 type;
  mColumn->GetType(&type);
  if (type == nsITreeColumn::TYPE_CHECKBOX && IsEditable())
    *aActionsCount = 1;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRBool isCycler = PR_FALSE;
  mColumn->GetCycler(&isCycler);
  if (isCycler) {
    aName.AssignLiteral("cycle");
    return NS_OK;
  }

  PRInt16 type;
  mColumn->GetType(&type);
  if (type == nsITreeColumn::TYPE_CHECKBOX && IsEditable()) {
    nsAutoString value;
    mTreeView->GetCellValue(mRow, mColumn, value);
    if (value.EqualsLiteral("true"))
      aName.AssignLiteral("uncheck");
    else
      aName.AssignLiteral("check");
    
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRBool isCycler = PR_FALSE;
  mColumn->GetCycler(&isCycler);
  if (isCycler) {
    DoCommand();
    return NS_OK;
  }

  PRInt16 type;
  mColumn->GetType(&type);
  if (type == nsITreeColumn::TYPE_CHECKBOX && IsEditable()) {
    DoCommand();
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}




NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetTable(nsIAccessibleTable **aTable)
{
  NS_ENSURE_ARG_POINTER(aTable);
  *aTable = nsnull;

  if (IsDefunct())
    return NS_OK;

  CallQueryInterface(mParent->GetParent(), aTable);
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetColumnIndex(PRInt32 *aColumnIndex)
{
  NS_ENSURE_ARG_POINTER(aColumnIndex);
  *aColumnIndex = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aColumnIndex = GetColumnIndex();
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetRowIndex(PRInt32 *aRowIndex)
{
  NS_ENSURE_ARG_POINTER(aRowIndex);
  *aRowIndex = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aRowIndex = mRow;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetColumnExtent(PRInt32 *aExtentCount)
{
  NS_ENSURE_ARG_POINTER(aExtentCount);
  *aExtentCount = 1;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetRowExtent(PRInt32 *aExtentCount)
{
  NS_ENSURE_ARG_POINTER(aExtentCount);
  *aExtentCount = 1;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetColumnHeaderCells(nsIArray **aHeaderCells)
{
  NS_ENSURE_ARG_POINTER(aHeaderCells);
  *aHeaderCells = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIMutableArray> headerCells =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMElement> columnElm;
  mColumn->GetElement(getter_AddRefs(columnElm));

  nsRefPtr<nsAccessible> headerCell =
    GetAccService()->GetAccessibleInWeakShell(columnElm, mWeakShell);

  if (headerCell)
    headerCells->AppendElement(static_cast<nsIAccessible*>(headerCell.get()),
                               PR_FALSE);

  NS_ADDREF(*aHeaderCells = headerCells);
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::GetRowHeaderCells(nsIArray **aHeaderCells)
{
  NS_ENSURE_ARG_POINTER(aHeaderCells);
  *aHeaderCells = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIMutableArray> headerCells =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aHeaderCells = headerCells);
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeGridCellAccessible::IsSelected(PRBool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = PR_FALSE;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  nsresult rv = mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  return selection->IsSelected(mRow, aIsSelected);
}




PRBool
nsXULTreeGridCellAccessible::IsDefunct()
{
  return nsLeafAccessible::IsDefunct() || !mParent || !mTree || !mTreeView ||
    !mColumn;
}

nsresult
nsXULTreeGridCellAccessible::Init()
{
  nsresult rv = nsLeafAccessible::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt16 type;
  mColumn->GetType(&type);
  if (type == nsITreeColumn::TYPE_CHECKBOX)
    mTreeView->GetCellValue(mRow, mColumn, mCachedTextEquiv);
  else
    mTreeView->GetCellText(mRow, mColumn, mCachedTextEquiv);

  return NS_OK;
}




nsresult
nsXULTreeGridCellAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIAccessible> accessible;
  mParent->GetParent(getter_AddRefs(accessible));
  nsCOMPtr<nsIAccessibleTable> tableAccessible = do_QueryInterface(accessible);

  
  if (!tableAccessible)
    return NS_ERROR_FAILURE;
    
  PRInt32 colIdx = GetColumnIndex();

  PRInt32 cellIdx = -1;
  tableAccessible->GetCellIndexAt(mRow, colIdx, &cellIdx);

  nsAutoString stringIdx;
  stringIdx.AppendInt(cellIdx);
  nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::tableCellIndex,
                         stringIdx);

  
  PRBool isCycler = PR_FALSE;
  nsresult rv = mColumn->GetCycler(&isCycler);
  if (NS_SUCCEEDED(rv) && isCycler)
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::cycles,
                           NS_LITERAL_STRING("true"));

  return NS_OK;
}

nsresult
nsXULTreeGridCellAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_GRID_CELL;
  return NS_OK;
}

nsresult
nsXULTreeGridCellAccessible::GetStateInternal(PRUint32 *aStates,
                                              PRUint32 *aExtraStates)
{
  NS_ENSURE_ARG_POINTER(aStates);

  *aStates = 0;
  if (aExtraStates)
    *aExtraStates = 0;

  if (IsDefunct()) {
    if (aExtraStates)
      *aExtraStates = nsIAccessibleStates::EXT_STATE_DEFUNCT;
    return NS_OK_DEFUNCT_OBJECT;
  }

  
  *aStates |= nsIAccessibleStates::STATE_SELECTABLE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected = PR_FALSE;
    selection->IsSelected(mRow, &isSelected);
    if (isSelected)
      *aStates |= nsIAccessibleStates::STATE_SELECTED;
  }

  
  PRInt16 type;
  mColumn->GetType(&type);
  if (type == nsITreeColumn::TYPE_CHECKBOX) {
    *aStates |= nsIAccessibleStates::STATE_CHECKABLE;
    nsAutoString checked;
    mTreeView->GetCellValue(mRow, mColumn, checked);
    if (checked.EqualsIgnoreCase("true"))
      *aStates |= nsIAccessibleStates::STATE_CHECKED;
  }

  return NS_OK;
}

nsAccessible*
nsXULTreeGridCellAccessible::GetParent()
{
  return IsDefunct() ? nsnull : mParent.get();
}




PRInt32
nsXULTreeGridCellAccessible::GetColumnIndex() const
{
  PRInt32 index = 0;
  nsCOMPtr<nsITreeColumn> column = mColumn;
  while (column = nsCoreUtils::GetPreviousSensibleColumn(column))
    index++;

  return index;
}

void
nsXULTreeGridCellAccessible::CellInvalidated()
{
  nsAutoString textEquiv;

  PRInt16 type;
  mColumn->GetType(&type);
  if (type == nsITreeColumn::TYPE_CHECKBOX) {
    mTreeView->GetCellValue(mRow, mColumn, textEquiv);
    if (mCachedTextEquiv != textEquiv) {
      PRBool isEnabled = textEquiv.EqualsLiteral("true");
      nsRefPtr<nsAccEvent> accEvent =
        new nsAccStateChangeEvent(this, nsIAccessibleStates::STATE_CHECKED,
                                  PR_FALSE, isEnabled);
      nsEventShell::FireEvent(accEvent);

      mCachedTextEquiv = textEquiv;
    }

    return;
  }

  mTreeView->GetCellText(mRow, mColumn, textEquiv);
  if (mCachedTextEquiv != textEquiv) {
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE, this);
    mCachedTextEquiv = textEquiv;
  }
}




nsAccessible*
nsXULTreeGridCellAccessible::GetSiblingAtOffset(PRInt32 aOffset,
                                                nsresult* aError)
{
  if (IsDefunct()) {
    if (aError)
      *aError = NS_ERROR_FAILURE;

    return nsnull;
  }

  if (aError)
    *aError = NS_OK; 

  nsCOMPtr<nsITreeColumn> columnAtOffset(mColumn), column;
  if (aOffset < 0) {
    for (PRInt32 index = aOffset; index < 0 && columnAtOffset; index++) {
      column = nsCoreUtils::GetPreviousSensibleColumn(columnAtOffset);
      column.swap(columnAtOffset);
    }
  } else {
    for (PRInt32 index = aOffset; index > 0 && columnAtOffset; index--) {
      column = nsCoreUtils::GetNextSensibleColumn(columnAtOffset);
      column.swap(columnAtOffset);
    }
  }

  if (!columnAtOffset)
    return nsnull;

  nsRefPtr<nsXULTreeItemAccessibleBase> rowAcc =
    nsAccUtils::QueryObject<nsXULTreeItemAccessibleBase>(mParent);

  return rowAcc->GetCellAccessible(columnAtOffset);
}

void
nsXULTreeGridCellAccessible::DispatchClickEvent(nsIContent *aContent,
                                                PRUint32 aActionIndex)
{
  if (IsDefunct())
    return;

  nsCoreUtils::DispatchClickEvent(mTree, mRow, mColumn);
}




PRBool
nsXULTreeGridCellAccessible::IsEditable() const
{
  
  
  PRBool isEditable = PR_FALSE;
  nsresult rv = mTreeView->IsEditable(mRow, mColumn, &isEditable);
  if (NS_FAILED(rv) || !isEditable)
    return PR_FALSE;

  nsCOMPtr<nsIDOMElement> columnElm;
  mColumn->GetElement(getter_AddRefs(columnElm));
  if (!columnElm)
    return PR_FALSE;

  nsCOMPtr<nsIContent> columnContent(do_QueryInterface(columnElm));
  if (!columnContent->AttrValueIs(kNameSpaceID_None,
                                  nsAccessibilityAtoms::editable,
                                  nsAccessibilityAtoms::_true,
                                  eCaseMatters))
    return PR_FALSE;

  nsCOMPtr<nsIContent> treeContent(do_QueryInterface(mDOMNode));
  return treeContent->AttrValueIs(kNameSpaceID_None,
                                  nsAccessibilityAtoms::editable,
                                  nsAccessibilityAtoms::_true,
                                  eCaseMatters);
}
