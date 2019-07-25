





































#include "nsXULTreeAccessible.h"

#include "nsAccCache.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"
#include "nsRelUtils.h"
#include "States.h"

#include "nsIDOMXULElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMXULTreeElement.h"
#include "nsITreeSelection.h"
#include "nsIMutableArray.h"
#include "nsComponentManagerUtils.h"





nsXULTreeAccessible::
  nsXULTreeAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
  mTree = nsCoreUtils::GetTreeBoxObject(aContent);
  if (mTree)
    mTree->GetView(getter_AddRefs(mTreeView));

  NS_ASSERTION(mTree && mTreeView, "Can't get mTree or mTreeView!\n");

  mAccessibleCache.Init(kDefaultTreeCacheSize);
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULTreeAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXULTreeAccessible,
                                                  nsAccessible)
CycleCollectorTraverseCache(tmp->mAccessibleCache, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXULTreeAccessible,
                                                nsAccessible)
ClearCache(tmp->mAccessibleCache);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXULTreeAccessible)
NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsXULTreeAccessible)
NS_INTERFACE_MAP_END_INHERITING(nsAccessible)

NS_IMPL_ADDREF_INHERITED(nsXULTreeAccessible, nsAccessible)
NS_IMPL_RELEASE_INHERITED(nsXULTreeAccessible, nsAccessible)




PRUint64
nsXULTreeAccessible::NativeState()
{
  
  PRUint64 state = nsAccessible::NativeState();

  
  state |= states::READONLY;

  
  state &= ~(states::FOCUSABLE | states::FOCUSED);

  
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_TRUE(selection, state);

  PRBool isSingle = PR_FALSE;
  nsresult rv = selection->GetSingle(&isSingle);
  NS_ENSURE_SUCCESS(rv, state);

  if (!isSingle)
    state |= states::MULTISELECTABLE;

  return state;
}

NS_IMETHODIMP
nsXULTreeAccessible::GetValue(nsAString& aValue)
{
  

  aValue.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (!selection)
    return NS_ERROR_FAILURE;

  PRInt32 currentIndex;
  nsCOMPtr<nsIDOMElement> selectItem;
  selection->GetCurrentIndex(&currentIndex);
  if (currentIndex >= 0) {
    nsCOMPtr<nsITreeColumn> keyCol;

    nsCOMPtr<nsITreeColumns> cols;
    mTree->GetColumns(getter_AddRefs(cols));
    if (cols)
      cols->GetKeyColumn(getter_AddRefs(keyCol));

    return mTreeView->GetCellText(currentIndex, keyCol, aValue);
  }

  return NS_OK;
}




PRBool
nsXULTreeAccessible::IsDefunct()
{
  return nsAccessibleWrap::IsDefunct() || !mTree || !mTreeView;
}

void
nsXULTreeAccessible::Shutdown()
{
  
  
  
  
  ClearCache(mAccessibleCache);

  mTree = nsnull;
  mTreeView = nsnull;

  nsAccessibleWrap::Shutdown();
}




PRUint32
nsXULTreeAccessible::NativeRole()
{
  
  

  nsCOMPtr<nsITreeColumns> cols;
  mTree->GetColumns(getter_AddRefs(cols));
  nsCOMPtr<nsITreeColumn> primaryCol;
  if (cols)
    cols->GetPrimaryColumn(getter_AddRefs(primaryCol));

  return primaryCol ?
    static_cast<PRUint32>(nsIAccessibleRole::ROLE_OUTLINE) :
    static_cast<PRUint32>(nsIAccessibleRole::ROLE_LIST);
}




NS_IMETHODIMP
nsXULTreeAccessible::GetFocusedChild(nsIAccessible **aFocusedChild) 
{
  NS_ENSURE_ARG_POINTER(aFocusedChild);
  *aFocusedChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (gLastFocusedNode != mContent)
    return NS_OK;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
    do_QueryInterface(mContent);
  if (multiSelect) {
    PRInt32 row = -1;
    multiSelect->GetCurrentIndex(&row);
    if (row >= 0)
      NS_IF_ADDREF(*aFocusedChild = GetTreeItemAccessible(row));
  }

  return NS_OK;
}




nsAccessible*
nsXULTreeAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     EWhichChildAtPoint aWhichChild)
{
  nsIFrame *frame = GetFrame();
  if (!frame)
    return nsnull;

  nsPresContext *presContext = frame->PresContext();
  nsCOMPtr<nsIPresShell> presShell = presContext->PresShell();

  nsIFrame *rootFrame = presShell->GetRootFrame();
  NS_ENSURE_TRUE(rootFrame, nsnull);

  nsIntRect rootRect = rootFrame->GetScreenRectExternal();

  PRInt32 clientX = presContext->DevPixelsToIntCSSPixels(aX - rootRect.x);
  PRInt32 clientY = presContext->DevPixelsToIntCSSPixels(aY - rootRect.y);

  PRInt32 row = -1;
  nsCOMPtr<nsITreeColumn> column;
  nsCAutoString childEltUnused;
  mTree->GetCellAt(clientX, clientY, &row, getter_AddRefs(column),
                   childEltUnused);

  
  
  if (row == -1 || !column)
    return nsAccessibleWrap::GetChildAtPoint(aX, aY, aWhichChild);

  nsAccessible *child = GetTreeItemAccessible(row);
  if (aWhichChild == eDeepestChild && child) {
    
    nsRefPtr<nsXULTreeItemAccessibleBase> treeitem = do_QueryObject(child);

    nsAccessible *cell = treeitem->GetCellAccessible(column);
    if (cell)
      child = cell;
  }

  return child;
}




bool
nsXULTreeAccessible::IsSelect()
{
  return true;
}

already_AddRefed<nsIArray>
nsXULTreeAccessible::SelectedItems()
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (!selection)
    return nsnull;

  nsCOMPtr<nsIMutableArray> selectedItems =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (!selectedItems)
    return nsnull;

  PRInt32 rangeCount = 0;
  selection->GetRangeCount(&rangeCount);
  for (PRInt32 rangeIdx = 0; rangeIdx < rangeCount; rangeIdx++) {
    PRInt32 firstIdx = 0, lastIdx = -1;
    selection->GetRangeAt(rangeIdx, &firstIdx, &lastIdx);
    for (PRInt32 rowIdx = firstIdx; rowIdx <= lastIdx; rowIdx++) {
      nsIAccessible* item = GetTreeItemAccessible(rowIdx);
      if (item)
        selectedItems->AppendElement(item, PR_FALSE);
    }
  }

  nsIMutableArray* items = nsnull;
  selectedItems.forget(&items);
  return items;
}

PRUint32
nsXULTreeAccessible::SelectedItemCount()
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRInt32 count = 0;
    selection->GetCount(&count);
    return count;
  }

  return 0;
}

bool
nsXULTreeAccessible::AddItemToSelection(PRUint32 aIndex)
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected = PR_FALSE;
    selection->IsSelected(aIndex, &isSelected);
    if (!isSelected)
      selection->ToggleSelect(aIndex);

    return true;
  }
  return false;
}

bool
nsXULTreeAccessible::RemoveItemFromSelection(PRUint32 aIndex)
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected = PR_FALSE;
    selection->IsSelected(aIndex, &isSelected);
    if (isSelected)
      selection->ToggleSelect(aIndex);

    return true;
  }
  return false;
}

bool
nsXULTreeAccessible::IsItemSelected(PRUint32 aIndex)
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected = PR_FALSE;
    selection->IsSelected(aIndex, &isSelected);
    return isSelected;
  }
  return false;
}

bool
nsXULTreeAccessible::UnselectAll()
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (!selection)
    return false;

  selection->ClearSelection();
  return true;
}

nsAccessible*
nsXULTreeAccessible::GetSelectedItem(PRUint32 aIndex)
{
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (!selection)
    return nsnull;

  PRUint32 selCount = 0;
  PRInt32 rangeCount = 0;
  selection->GetRangeCount(&rangeCount);
  for (PRInt32 rangeIdx = 0; rangeIdx < rangeCount; rangeIdx++) {
    PRInt32 firstIdx = 0, lastIdx = -1;
    selection->GetRangeAt(rangeIdx, &firstIdx, &lastIdx);
    for (PRInt32 rowIdx = firstIdx; rowIdx <= lastIdx; rowIdx++) {
      if (selCount == aIndex)
        return GetTreeItemAccessible(rowIdx);

      selCount++;
    }
  }

  return nsnull;
}

bool
nsXULTreeAccessible::SelectAll()
{
  
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool single = PR_FALSE;
    selection->GetSingle(&single);
    if (!single) {
      selection->SelectAll();
      return true;
    }
  }

  return false;
}




nsAccessible*
nsXULTreeAccessible::GetChildAt(PRUint32 aIndex)
{
  PRInt32 childCount = nsAccessible::GetChildCount();
  if (childCount == -1)
    return nsnull;

  if (static_cast<PRInt32>(aIndex) < childCount)
    return nsAccessible::GetChildAt(aIndex);

  return GetTreeItemAccessible(aIndex - childCount);
}

PRInt32
nsXULTreeAccessible::GetChildCount()
{
  
  PRInt32 childCount = nsAccessible::GetChildCount();
  if (childCount == -1)
    return -1;

  PRInt32 rowCount = 0;
  mTreeView->GetRowCount(&rowCount);
  childCount += rowCount;

  return childCount;
}




nsAccessible*
nsXULTreeAccessible::GetTreeItemAccessible(PRInt32 aRow)
{
  if (aRow < 0 || IsDefunct())
    return nsnull;

  PRInt32 rowCount = 0;
  nsresult rv = mTreeView->GetRowCount(&rowCount);
  if (NS_FAILED(rv) || aRow >= rowCount)
    return nsnull;

  void *key = reinterpret_cast<void*>(aRow);
  nsAccessible* cachedTreeItem = mAccessibleCache.GetWeak(key);
  if (cachedTreeItem)
    return cachedTreeItem;

  nsRefPtr<nsAccessible> treeItem = CreateTreeItemAccessible(aRow);
  if (treeItem) {
    if (mAccessibleCache.Put(key, treeItem)) {
      if (GetDocAccessible()->BindToDocument(treeItem, nsnull))
        return treeItem;

      mAccessibleCache.Remove(key);
    }
  }

  return nsnull;
}

void
nsXULTreeAccessible::InvalidateCache(PRInt32 aRow, PRInt32 aCount)
{
  if (IsDefunct())
    return;

  
  if (aCount > 0)
    return;

  nsDocAccessible* document = GetDocAccessible();

  
  for (PRInt32 rowIdx = aRow; rowIdx < aRow - aCount; rowIdx++) {

    void* key = reinterpret_cast<void*>(rowIdx);
    nsAccessible* treeItem = mAccessibleCache.GetWeak(key);

    if (treeItem) {
      nsRefPtr<AccEvent> event =
        new AccEvent(nsIAccessibleEvent::EVENT_HIDE, treeItem);
      nsEventShell::FireEvent(event);

      
      document->UnbindFromDocument(treeItem);
      mAccessibleCache.Remove(key);
    }
  }

  
  
  
  PRInt32 newRowCount = 0;
  nsresult rv = mTreeView->GetRowCount(&newRowCount);
  if (NS_FAILED(rv))
    return;

  PRInt32 oldRowCount = newRowCount - aCount;

  for (PRInt32 rowIdx = newRowCount; rowIdx < oldRowCount; ++rowIdx) {

    void *key = reinterpret_cast<void*>(rowIdx);
    nsAccessible* treeItem = mAccessibleCache.GetWeak(key);

    if (treeItem) {
      
      document->UnbindFromDocument(treeItem);
      mAccessibleCache.Remove(key);
    }
  }
}

void
nsXULTreeAccessible::TreeViewInvalidated(PRInt32 aStartRow, PRInt32 aEndRow,
                                         PRInt32 aStartCol, PRInt32 aEndCol)
{
  if (IsDefunct())
    return;

  PRInt32 endRow = aEndRow;

  nsresult rv;
  if (endRow == -1) {
    PRInt32 rowCount = 0;
    rv = mTreeView->GetRowCount(&rowCount);
    if (NS_FAILED(rv))
      return;

    endRow = rowCount - 1;
  }

  nsCOMPtr<nsITreeColumns> treeColumns;
  mTree->GetColumns(getter_AddRefs(treeColumns));
  if (!treeColumns)
    return;

  PRInt32 endCol = aEndCol;

  if (endCol == -1) {
    PRInt32 colCount = 0;
    rv = treeColumns->GetCount(&colCount);
    if (NS_FAILED(rv))
      return;

    endCol = colCount - 1;
  }

  for (PRInt32 rowIdx = aStartRow; rowIdx <= endRow; ++rowIdx) {

    void *key = reinterpret_cast<void*>(rowIdx);
    nsAccessible *accessible = mAccessibleCache.GetWeak(key);

    if (accessible) {
      nsRefPtr<nsXULTreeItemAccessibleBase> treeitemAcc = do_QueryObject(accessible);
      NS_ASSERTION(treeitemAcc, "Wrong accessible at the given key!");

      treeitemAcc->RowInvalidated(aStartCol, endCol);
    }
  }
}

void
nsXULTreeAccessible::TreeViewChanged()
{
  if (IsDefunct())
    return;

  
  
  
  nsRefPtr<AccEvent> reorderEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_REORDER, this, eAutoDetect,
                 AccEvent::eCoalesceFromSameSubtree);
  if (reorderEvent)
    GetDocAccessible()->FireDelayedAccessibleEvent(reorderEvent);

  
  ClearCache(mAccessibleCache);
  mTree->GetView(getter_AddRefs(mTreeView));
}




already_AddRefed<nsAccessible>
nsXULTreeAccessible::CreateTreeItemAccessible(PRInt32 aRow)
{
  nsRefPtr<nsAccessible> accessible =
    new nsXULTreeItemAccessible(mContent, mWeakShell, this, mTree, mTreeView,
                                aRow);

  return accessible.forget();
}
                             




nsXULTreeItemAccessibleBase::
  nsXULTreeItemAccessibleBase(nsIContent *aContent, nsIWeakReference *aShell,
                              nsAccessible *aParent, nsITreeBoxObject *aTree,
                              nsITreeView *aTreeView, PRInt32 aRow) :
  nsAccessibleWrap(aContent, aShell),
  mTree(aTree), mTreeView(aTreeView), mRow(aRow)
{
  mParent = aParent;
}




NS_IMPL_ISUPPORTS_INHERITED1(nsXULTreeItemAccessibleBase,
                             nsAccessible,
                             nsXULTreeItemAccessibleBase)




NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetFocusedChild(nsIAccessible **aFocusedChild) 
{
  NS_ENSURE_ARG_POINTER(aFocusedChild);
  *aFocusedChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (gLastFocusedNode != mContent)
    return NS_OK;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
    do_QueryInterface(mContent);

  if (multiSelect) {
    PRInt32 row = -1;
    multiSelect->GetCurrentIndex(&row);
    if (row == mRow)
      NS_ADDREF(*aFocusedChild = this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetBounds(PRInt32 *aX, PRInt32 *aY,
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

  nsCOMPtr<nsITreeColumn> column = nsCoreUtils::GetFirstSensibleColumn(mTree);

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = mTree->GetCoordsForCellItem(mRow, column, EmptyCString(),
                                            &x, &y, &width, &height);
  NS_ENSURE_SUCCESS(rv, rv);

  boxObj->GetWidth(&width);

  PRInt32 tcX = 0, tcY = 0;
  boxObj->GetScreenX(&tcX);
  boxObj->GetScreenY(&tcY);

  x = tcX;
  y += tcY;

  nsPresContext *presContext = GetPresContext();
  *aX = presContext->CSSPixelsToDevPixels(x);
  *aY = presContext->CSSPixelsToDevPixels(y);
  *aWidth = presContext->CSSPixelsToDevPixels(width);
  *aHeight = presContext->CSSPixelsToDevPixels(height);

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::SetSelected(PRBool aSelect)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected;
    selection->IsSelected(mRow, &isSelected);
    if (isSelected != aSelect)
      selection->ToggleSelect(mRow);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::TakeFocus()
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection)
    selection->SetCurrentIndex(mRow);

  
  return nsAccessible::TakeFocus();
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetRelationByType(PRUint32 aRelationType,
                                               nsIAccessibleRelation **aRelation)
{
  NS_ENSURE_ARG_POINTER(aRelation);
  *aRelation = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aRelationType == nsIAccessibleRelation::RELATION_NODE_CHILD_OF) {
    PRInt32 parentIndex;
    if (NS_SUCCEEDED(mTreeView->GetParentIndex(mRow, &parentIndex))) {
      if (parentIndex == -1)
        return nsRelUtils::AddTarget(aRelationType, aRelation, mParent);

      nsRefPtr<nsXULTreeAccessible> treeAcc = do_QueryObject(mParent);

      nsAccessible *logicalParent = treeAcc->GetTreeItemAccessible(parentIndex);
      return nsRelUtils::AddTarget(aRelationType, aRelation, logicalParent);
    }

    return NS_OK;
  }

  return nsAccessible::GetRelationByType(aRelationType, aRelation);
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetNumActions(PRUint8 *aActionsCount)
{
  NS_ENSURE_ARG_POINTER(aActionsCount);
  *aActionsCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  
  *aActionsCount = IsExpandable() ? 2 : 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aIndex == eAction_Click) {
    aName.AssignLiteral("activate");
    return NS_OK;
  }

  if (aIndex == eAction_Expand && IsExpandable()) {
    PRBool isContainerOpen;
    mTreeView->IsContainerOpen(mRow, &isContainerOpen);
    if (isContainerOpen)
      aName.AssignLiteral("collapse");
    else
      aName.AssignLiteral("expand");

    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::DoAction(PRUint8 aIndex)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aIndex != eAction_Click &&
      (aIndex != eAction_Expand || !IsExpandable()))
    return NS_ERROR_INVALID_ARG;

  DoCommand(nsnull, aIndex);
  return NS_OK;
}




PRBool
nsXULTreeItemAccessibleBase::IsDefunct()
{
  if (nsAccessibleWrap::IsDefunct() || !mTree || !mTreeView || mRow < 0)
    return PR_TRUE;

  PRInt32 rowCount = 0;
  nsresult rv = mTreeView->GetRowCount(&rowCount);
  return NS_FAILED(rv) || mRow >= rowCount;
}

void
nsXULTreeItemAccessibleBase::Shutdown()
{
  mTree = nsnull;
  mTreeView = nsnull;
  mRow = -1;

  nsAccessibleWrap::Shutdown();
}

bool
nsXULTreeItemAccessibleBase::IsPrimaryForNode() const
{
  return false;
}





nsresult
nsXULTreeItemAccessibleBase::GroupPosition(PRInt32 *aGroupLevel,
                                           PRInt32 *aSimilarItemsInGroup,
                                           PRInt32 *aPositionInGroup)
{
  NS_ENSURE_ARG_POINTER(aGroupLevel);
  *aGroupLevel = 0;

  NS_ENSURE_ARG_POINTER(aSimilarItemsInGroup);
  *aSimilarItemsInGroup = 0;

  NS_ENSURE_ARG_POINTER(aPositionInGroup);
  *aPositionInGroup = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 level;
  nsresult rv = mTreeView->GetLevel(mRow, &level);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 topCount = 1;
  for (PRInt32 index = mRow - 1; index >= 0; index--) {
    PRInt32 lvl = -1;
    if (NS_SUCCEEDED(mTreeView->GetLevel(index, &lvl))) {
      if (lvl < level)
        break;

      if (lvl == level)
        topCount++;
    }
  }

  PRInt32 rowCount = 0;
  rv = mTreeView->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 bottomCount = 0;
  for (PRInt32 index = mRow + 1; index < rowCount; index++) {
    PRInt32 lvl = -1;
    if (NS_SUCCEEDED(mTreeView->GetLevel(index, &lvl))) {
      if (lvl < level)
        break;

      if (lvl == level)
        bottomCount++;
    }
  }

  PRInt32 setSize = topCount + bottomCount;
  PRInt32 posInSet = topCount;

  *aGroupLevel = level + 1;
  *aSimilarItemsInGroup = setSize;
  *aPositionInGroup = posInSet;

  return NS_OK;
}

PRUint64
nsXULTreeItemAccessibleBase::NativeState()
{

  if (IsDefunct())
    return states::DEFUNCT;

  
  PRUint64 state = states::FOCUSABLE | states::SELECTABLE;

  
  if (IsExpandable()) {
    PRBool isContainerOpen;
    mTreeView->IsContainerOpen(mRow, &isContainerOpen);
    state |= isContainerOpen ? states::EXPANDED : states::COLLAPSED;
  }

  
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected;
    selection->IsSelected(mRow, &isSelected);
    if (isSelected)
      state |= states::SELECTED;
  }

  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
    do_QueryInterface(mContent);
  if (multiSelect) {
    PRInt32 currentIndex;
    multiSelect->GetCurrentIndex(&currentIndex);
    if (currentIndex == mRow) {
      state |= states::FOCUSED;
    }
  }

  
  PRInt32 firstVisibleRow, lastVisibleRow;
  mTree->GetFirstVisibleRow(&firstVisibleRow);
  mTree->GetLastVisibleRow(&lastVisibleRow);
  if (mRow < firstVisibleRow || mRow > lastVisibleRow)
    state |= states::INVISIBLE;

  return state;
}

PRInt32
nsXULTreeItemAccessibleBase::GetIndexInParent() const
{
  return mParent ? mParent->GetCachedChildCount() + mRow : -1;
}




void
nsXULTreeItemAccessibleBase::DispatchClickEvent(nsIContent *aContent,
                                                PRUint32 aActionIndex)
{
  if (IsDefunct())
    return;

  nsCOMPtr<nsITreeColumns> columns;
  mTree->GetColumns(getter_AddRefs(columns));
  if (!columns)
    return;

  
  nsCOMPtr<nsITreeColumn> column;
  nsCAutoString pseudoElm;

  if (aActionIndex == eAction_Click) {
    
    columns->GetKeyColumn(getter_AddRefs(column));
  } else {
    
    columns->GetPrimaryColumn(getter_AddRefs(column));
    pseudoElm = NS_LITERAL_CSTRING("twisty");
  }

  if (column)
    nsCoreUtils::DispatchClickEvent(mTree, mRow, column, pseudoElm);
}

nsAccessible*
nsXULTreeItemAccessibleBase::GetSiblingAtOffset(PRInt32 aOffset,
                                                nsresult* aError)
{
  if (IsDefunct()) {
    if (aError)
      *aError = NS_ERROR_FAILURE;

    return nsnull;
  }

  if (aError)
    *aError = NS_OK; 

  return mParent->GetChildAt(GetIndexInParent() + aOffset);
}




PRBool
nsXULTreeItemAccessibleBase::IsExpandable()
{
  PRBool isContainer = PR_FALSE;
  mTreeView->IsContainer(mRow, &isContainer);
  if (isContainer) {
    PRBool isEmpty = PR_FALSE;
    mTreeView->IsContainerEmpty(mRow, &isEmpty);
    if (!isEmpty) {
      nsCOMPtr<nsITreeColumns> columns;
      mTree->GetColumns(getter_AddRefs(columns));
      nsCOMPtr<nsITreeColumn> primaryColumn;
      if (columns) {
        columns->GetPrimaryColumn(getter_AddRefs(primaryColumn));
        if (primaryColumn &&
            !nsCoreUtils::IsColumnHidden(primaryColumn))
          return PR_TRUE;
      }
    }
  }

  return PR_FALSE;
}






nsXULTreeItemAccessible::
  nsXULTreeItemAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                          nsAccessible *aParent, nsITreeBoxObject *aTree,
                          nsITreeView *aTreeView, PRInt32 aRow) :
  nsXULTreeItemAccessibleBase(aContent, aShell, aParent, aTree, aTreeView, aRow)
{
  mColumn = nsCoreUtils::GetFirstSensibleColumn(mTree);
}




NS_IMETHODIMP
nsXULTreeItemAccessible::GetName(nsAString& aName)
{
  aName.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  mTreeView->GetCellText(mRow, mColumn, aName);

  
  
  
  
  
  if (aName.IsEmpty())
    mTreeView->GetCellValue(mRow, mColumn, aName);

  return NS_OK;
}




PRBool
nsXULTreeItemAccessible::IsDefunct()
{
  return nsXULTreeItemAccessibleBase::IsDefunct() || !mColumn;
}

PRBool
nsXULTreeItemAccessible::Init()
{
  if (!nsXULTreeItemAccessibleBase::Init())
    return PR_FALSE;

  GetName(mCachedName);
  return PR_TRUE;
}

void
nsXULTreeItemAccessible::Shutdown()
{
  mColumn = nsnull;
  nsXULTreeItemAccessibleBase::Shutdown();
}




PRUint32
nsXULTreeItemAccessible::NativeRole()
{
  nsCOMPtr<nsITreeColumns> columns;
  mTree->GetColumns(getter_AddRefs(columns));
  if (!columns) {
    NS_ERROR("No tree columns object in the tree!");
    return nsIAccessibleRole::ROLE_NOTHING;
  }

  nsCOMPtr<nsITreeColumn> primaryColumn;
  columns->GetPrimaryColumn(getter_AddRefs(primaryColumn));

  return primaryColumn ?
    static_cast<PRUint32>(nsIAccessibleRole::ROLE_OUTLINEITEM) :
    static_cast<PRUint32>(nsIAccessibleRole::ROLE_LISTITEM);
}




void
nsXULTreeItemAccessible::RowInvalidated(PRInt32 aStartColIdx,
                                        PRInt32 aEndColIdx)
{
  nsAutoString name;
  GetName(name);

  if (name != mCachedName) {
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE, this);
    mCachedName = name;
  }
}




void
nsXULTreeItemAccessible::CacheChildren()
{
}






nsXULTreeColumnsAccessible::
  nsXULTreeColumnsAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULColumnsAccessible(aContent, aShell)
{
}

nsAccessible*
nsXULTreeColumnsAccessible::GetSiblingAtOffset(PRInt32 aOffset,
                                               nsresult* aError)
{
  if (aOffset < 0)
    return nsXULColumnsAccessible::GetSiblingAtOffset(aOffset, aError);

  if (IsDefunct()) {
    if (aError)
      *aError = NS_ERROR_FAILURE;

    return nsnull;
  }

  if (aError)
    *aError = NS_OK; 

  nsCOMPtr<nsITreeBoxObject> tree = nsCoreUtils::GetTreeBoxObject(mContent);
  if (tree) {
    nsCOMPtr<nsITreeView> treeView;
    tree->GetView(getter_AddRefs(treeView));
    if (treeView) {
      PRInt32 rowCount = 0;
      treeView->GetRowCount(&rowCount);
      if (rowCount > 0 && aOffset <= rowCount) {
        nsRefPtr<nsXULTreeAccessible> treeAcc = do_QueryObject(mParent);

        if (treeAcc)
          return treeAcc->GetTreeItemAccessible(aOffset - 1);
      }
    }
  }

  return nsnull;
}

