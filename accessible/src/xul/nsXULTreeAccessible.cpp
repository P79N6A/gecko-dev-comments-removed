





































#include "nsXULTreeAccessible.h"

#include "nsDocAccessible.h"

#include "nsIDOMXULElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMXULTreeElement.h"
#include "nsITreeSelection.h"
#include "nsIMutableArray.h"
#include "nsComponentManagerUtils.h"





static PLDHashOperator
ElementTraverser(const void *aKey, nsIAccessNode *aAccessNode,
                 void *aUserArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aUserArg);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mAccessNodeCache of XUL tree entry");
  cb->NoteXPCOMChild(aAccessNode);
  return PL_DHASH_NEXT;
}






nsXULTreeAccessible::
  nsXULTreeAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell) :
  nsXULSelectableAccessible(aDOMNode, aShell)
{
  nsCoreUtils::GetTreeBoxObject(aDOMNode, getter_AddRefs(mTree));
  if (mTree)
    mTree->GetView(getter_AddRefs(mTreeView));

  NS_ASSERTION(mTree && mTreeView, "Can't get mTree or mTreeView!\n");

  mAccessNodeCache.Init(kDefaultTreeCacheSize);
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULTreeAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXULTreeAccessible,
                                                  nsAccessible)
tmp->mAccessNodeCache.EnumerateRead(ElementTraverser, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXULTreeAccessible,
                                                nsAccessible)
tmp->ClearCache(tmp->mAccessNodeCache);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXULTreeAccessible)
NS_INTERFACE_MAP_STATIC_AMBIGUOUS(nsXULTreeAccessible)
NS_INTERFACE_MAP_END_INHERITING(nsXULSelectableAccessible)

NS_IMPL_ADDREF_INHERITED(nsXULTreeAccessible, nsXULSelectableAccessible)
NS_IMPL_RELEASE_INHERITED(nsXULTreeAccessible, nsXULSelectableAccessible)




nsresult
nsXULTreeAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  *aState |= nsIAccessibleStates::STATE_READONLY;

  
  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  *aState &= ~nsIAccessibleStates::STATE_FOCUSED;

  
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_STATE(selection);

  PRBool isSingle = PR_FALSE;
  rv = selection->GetSingle(&isSingle);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!isSingle)
    *aState |= nsIAccessibleStates::STATE_MULTISELECTABLE;

  return NS_OK;
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
  return nsXULSelectableAccessible::IsDefunct() || !mTree || !mTreeView;
}

nsresult
nsXULTreeAccessible::Shutdown()
{
  
  
  
  
  ClearCache(mAccessNodeCache);

  mTree = nsnull;
  mTreeView = nsnull;

  nsXULSelectableAccessible::Shutdown();
  return NS_OK;
}




nsresult
nsXULTreeAccessible::GetRoleInternal(PRUint32 *aRole)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsITreeColumns> cols;
  mTree->GetColumns(getter_AddRefs(cols));
  nsCOMPtr<nsITreeColumn> primaryCol;
  if (cols)
    cols->GetPrimaryColumn(getter_AddRefs(primaryCol));

  *aRole = primaryCol ?
    nsIAccessibleRole::ROLE_OUTLINE :
    nsIAccessibleRole::ROLE_LIST;

  return NS_OK;
}




NS_IMETHODIMP
nsXULTreeAccessible::GetFirstChild(nsIAccessible **aFirstChild)
{
  nsAccessible::GetFirstChild(aFirstChild);

  
  
  if (*aFirstChild == nsnull) {
    if (IsDefunct())
      return NS_ERROR_FAILURE;

    PRInt32 rowCount;
    mTreeView->GetRowCount(&rowCount);
    if (rowCount > 0)
      GetTreeItemAccessible(0, aFirstChild);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeAccessible::GetLastChild(nsIAccessible **aLastChild)
{
  NS_ENSURE_ARG_POINTER(aLastChild);
  *aLastChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 rowCount = 0;
  mTreeView->GetRowCount(&rowCount);
  if (rowCount > 0) {
    GetTreeItemAccessible(rowCount - 1, aLastChild);

    if (*aLastChild)
      return NS_OK;
  }

  
  return nsAccessible::GetLastChild(aLastChild);
}

NS_IMETHODIMP
nsXULTreeAccessible::GetChildCount(PRInt32 *aChildCount)
{
  
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAccessible::GetChildCount(aChildCount);

  if (*aChildCount != eChildCountUninitialized) {
    PRInt32 rowCount = 0;
    mTreeView->GetRowCount(&rowCount);
    *aChildCount += rowCount;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeAccessible::GetChildAt(PRInt32 aChildIndex, nsIAccessible **aChild)
{
  NS_ENSURE_ARG_POINTER(aChild);
  *aChild = nsnull;

  PRInt32 childCount = 0;
  nsresult rv = nsAccessible::GetChildCount(&childCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aChildIndex < childCount)
    return nsAccessible::GetChildAt(aChildIndex, aChild);

  GetTreeItemAccessible(aChildIndex - childCount, aChild);
  return *aChild ? NS_OK : NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsXULTreeAccessible::GetFocusedChild(nsIAccessible **aFocusedChild) 
{
  NS_ENSURE_ARG_POINTER(aFocusedChild);
  *aFocusedChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (gLastFocusedNode != mDOMNode)
    return NS_OK;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
    do_QueryInterface(mDOMNode);
  if (multiSelect) {
    PRInt32 row = -1;
    multiSelect->GetCurrentIndex(&row);
    if (row >= 0)
      GetTreeItemAccessible(row, aFocusedChild);
  }

  return NS_OK;
}




nsresult
nsXULTreeAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
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

  
  
  if (row == -1 || !column)
    return nsXULSelectableAccessible::
      GetChildAtPoint(aX, aY, aDeepestChild, aChild);

  GetTreeItemAccessible(row, aChild);
  if (aDeepestChild && *aChild) {
    
    nsRefPtr<nsXULTreeItemAccessibleBase> treeitemAcc =
      nsAccUtils::QueryObject<nsXULTreeItemAccessibleBase>(*aChild);

    nsCOMPtr<nsIAccessible> cellAccessible;
    treeitemAcc->GetCellAccessible(column, getter_AddRefs(cellAccessible));
    if (cellAccessible)
      cellAccessible.swap(*aChild);
  }

  return NS_OK;
}




NS_IMETHODIMP nsXULTreeAccessible::GetSelectedChildren(nsIArray **_retval)
{
  
  *_retval = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (!selection)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIMutableArray> selectedAccessibles =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_STATE(selectedAccessibles);

  PRInt32 rowIndex, rowCount;
  PRBool isSelected;
  mTreeView->GetRowCount(&rowCount);
  for (rowIndex = 0; rowIndex < rowCount; rowIndex++) {
    selection->IsSelected(rowIndex, &isSelected);
    if (isSelected) {
      nsCOMPtr<nsIAccessible> tempAccess;
      GetTreeItemAccessible(rowIndex, getter_AddRefs(tempAccess));
      NS_ENSURE_STATE(tempAccess);

      selectedAccessibles->AppendElement(tempAccess, PR_FALSE);
    }
  }

  PRUint32 length;
  selectedAccessibles->GetLength(&length);
  if (length != 0) {
    *_retval = selectedAccessibles;
    NS_IF_ADDREF(*_retval);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  *aSelectionCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection)
    selection->GetCount(aSelectionCount);

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    selection->IsSelected(aIndex, aSelState);
    if ((!(*aSelState) && eSelection_Add == aMethod) || 
        ((*aSelState) && eSelection_Remove == aMethod))
      return selection->ToggleSelect(aIndex);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXULTreeAccessible::AddChildToSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Add, &isSelected);
}

NS_IMETHODIMP nsXULTreeAccessible::RemoveChildFromSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Remove, &isSelected);
}

NS_IMETHODIMP nsXULTreeAccessible::IsChildSelected(PRInt32 aIndex, PRBool *_retval)
{
  return ChangeSelection(aIndex, eSelection_GetState, _retval);
}

NS_IMETHODIMP nsXULTreeAccessible::ClearSelection()
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection)
    selection->ClearSelection();

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeAccessible::RefSelection(PRInt32 aIndex, nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (!selection)
    return NS_ERROR_FAILURE;

  PRInt32 rowIndex, rowCount;
  PRInt32 selCount = 0;
  PRBool isSelected;
  mTreeView->GetRowCount(&rowCount);
  for (rowIndex = 0; rowIndex < rowCount; rowIndex++) {
    selection->IsSelected(rowIndex, &isSelected);
    if (isSelected) {
      if (selCount == aIndex) {
        GetTreeItemAccessible(rowIndex, aAccessible);
        return NS_OK;
      }
      selCount++;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeAccessible::SelectAllSelection(PRBool *aIsMultiSelectable)
{
  NS_ENSURE_ARG_POINTER(aIsMultiSelectable);
  *aIsMultiSelectable = PR_FALSE;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMElement> element (do_QueryInterface(mDOMNode));
  if (element) {
    nsCOMPtr<nsITreeSelection> selection;
    mTreeView->GetSelection(getter_AddRefs(selection));
    if (selection) {
      PRBool single = PR_FALSE;
      selection->GetSingle(&single);
      if (!single) {
        *aIsMultiSelectable = PR_TRUE;
        selection->SelectAll();
      }
    }
  }

  return NS_OK;
}




void
nsXULTreeAccessible::GetTreeItemAccessible(PRInt32 aRow,
                                           nsIAccessible** aAccessible)
{
  *aAccessible = nsnull;

  if (aRow < 0)
    return;

  PRInt32 rowCount = 0;
  nsresult rv = mTreeView->GetRowCount(&rowCount);
  if (NS_FAILED(rv) || aRow >= rowCount)
    return;

  void *key = reinterpret_cast<void*>(aRow);
  nsCOMPtr<nsIAccessNode> accessNode;
  GetCacheEntry(mAccessNodeCache, key, getter_AddRefs(accessNode));

  if (!accessNode) {
    nsRefPtr<nsAccessNode> treeItemAcc;
    CreateTreeItemAccessible(aRow, getter_AddRefs(treeItemAcc));
    if (!treeItemAcc)
      return;

    nsresult rv = treeItemAcc->Init();
    if (NS_FAILED(rv))
      return;

    accessNode = treeItemAcc;
    PutCacheEntry(mAccessNodeCache, key, accessNode);
  }

  CallQueryInterface(accessNode, aAccessible);
}

void
nsXULTreeAccessible::InvalidateCache(PRInt32 aRow, PRInt32 aCount)
{
  if (IsDefunct())
    return;

  
  if (aCount > 0)
    return;

  
  for (PRInt32 rowIdx = aRow; rowIdx < aRow - aCount; rowIdx++) {
    void *key = reinterpret_cast<void*>(rowIdx);

    nsCOMPtr<nsIAccessNode> accessNode;
    GetCacheEntry(mAccessNodeCache, key, getter_AddRefs(accessNode));

    if (accessNode) {
      nsRefPtr<nsAccessible> accessible =
        nsAccUtils::QueryAccessible(accessNode);

      nsCOMPtr<nsIAccessibleEvent> event =
        new nsAccEvent(nsIAccessibleEvent::EVENT_HIDE, accessible, PR_FALSE);
      FireAccessibleEvent(event);

      accessible->Shutdown();

      
      nsCOMPtr<nsIAccessibleDocument> docAccessible = GetDocAccessible();
      if (docAccessible) { 
        nsRefPtr<nsDocAccessible> docAcc =
          nsAccUtils::QueryAccessibleDocument(docAccessible);
        docAcc->RemoveAccessNodeFromCache(accessible);
      }

      mAccessNodeCache.Remove(key);
    }
  }

  
  
  
  PRInt32 newRowCount = 0;
  nsresult rv = mTreeView->GetRowCount(&newRowCount);
  if (NS_FAILED(rv))
    return;

  PRInt32 oldRowCount = newRowCount - aCount;

  for (PRInt32 rowIdx = newRowCount; rowIdx < oldRowCount; ++rowIdx) {
    void *key = reinterpret_cast<void*>(rowIdx);

    nsCOMPtr<nsIAccessNode> accessNode;
    GetCacheEntry(mAccessNodeCache, key, getter_AddRefs(accessNode));

    if (accessNode) {
      nsRefPtr<nsAccessNode> accNode =
        nsAccUtils::QueryAccessNode(accessNode);

      accNode->Shutdown();

      
      nsCOMPtr<nsIAccessibleDocument> docAccessible = GetDocAccessible();
      if (docAccessible) {
        nsRefPtr<nsDocAccessible> docAcc =
          nsAccUtils::QueryAccessibleDocument(docAccessible);
        docAcc->RemoveAccessNodeFromCache(accNode);
      }

      mAccessNodeCache.Remove(key);
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

    nsCOMPtr<nsIAccessNode> accessNode;
    GetCacheEntry(mAccessNodeCache, key, getter_AddRefs(accessNode));

    if (accessNode) {
      nsRefPtr<nsXULTreeItemAccessibleBase> treeitemAcc =
        nsAccUtils::QueryObject<nsXULTreeItemAccessibleBase>(accessNode);
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

  
  
  
  nsCOMPtr<nsIAccessibleEvent> eventDestroy =
    new nsAccEvent(nsIAccessibleEvent::EVENT_HIDE, this, PR_FALSE);
  if (!eventDestroy)
    return;

  FirePlatformEvent(eventDestroy);

  ClearCache(mAccessNodeCache);

  mTree->GetView(getter_AddRefs(mTreeView));

  nsCOMPtr<nsIAccessibleEvent> eventCreate =
    new nsAccEvent(nsIAccessibleEvent::EVENT_SHOW, this, PR_FALSE);
  if (!eventCreate)
    return;

  FirePlatformEvent(eventCreate);
}




void
nsXULTreeAccessible::CreateTreeItemAccessible(PRInt32 aRow,
                                              nsAccessNode** aAccessNode)
{
  *aAccessNode = new nsXULTreeItemAccessible(mDOMNode, mWeakShell, this,
                                             mTree, mTreeView, aRow);
  NS_IF_ADDREF(*aAccessNode);
}
                             




nsXULTreeItemAccessibleBase::
  nsXULTreeItemAccessibleBase(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                              nsIAccessible *aParent, nsITreeBoxObject *aTree,
                              nsITreeView *aTreeView, PRInt32 aRow) :
  mTree(aTree), mTreeView(aTreeView), mRow(aRow),
  nsAccessibleWrap(aDOMNode, aShell)
{
  mParent = aParent;
}




NS_IMPL_ISUPPORTS_INHERITED1(nsXULTreeItemAccessibleBase,
                             nsAccessible,
                             nsXULTreeItemAccessibleBase)




NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetUniqueID(void **aUniqueID)
{
  
  
  *aUniqueID = static_cast<void*>(this);
  return NS_OK;
}




NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetParent(nsIAccessible **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);
  *aParent = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (mParent) {
    *aParent = mParent;
    NS_ADDREF(*aParent);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetNextSibling(nsIAccessible **aNextSibling)
{
  NS_ENSURE_ARG_POINTER(aNextSibling);
  *aNextSibling = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsRefPtr<nsXULTreeAccessible> treeAcc =
    nsAccUtils::QueryAccessibleTree(mParent);
  NS_ENSURE_STATE(treeAcc);

  PRInt32 rowCount = 0;
  mTreeView->GetRowCount(&rowCount);
  if (mRow + 1 >= rowCount)
    return NS_OK;

  treeAcc->GetTreeItemAccessible(mRow + 1, aNextSibling);
  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetPreviousSibling(nsIAccessible **aPreviousSibling)
{
  NS_ENSURE_ARG_POINTER(aPreviousSibling);
  *aPreviousSibling = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsRefPtr<nsXULTreeAccessible> treeAcc =
    nsAccUtils::QueryAccessibleTree(mParent);
  NS_ENSURE_STATE(treeAcc);

  
  if (mRow > 0)
    treeAcc->GetTreeItemAccessible(mRow - 1, aPreviousSibling);
  else
    treeAcc->GetFirstChild(aPreviousSibling);

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessibleBase::GetFocusedChild(nsIAccessible **aFocusedChild) 
{
  NS_ENSURE_ARG_POINTER(aFocusedChild);
  *aFocusedChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (gLastFocusedNode != mDOMNode)
    return NS_OK;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
    do_QueryInterface(mDOMNode);

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

      nsRefPtr<nsXULTreeAccessible> treeAcc =
        nsAccUtils::QueryAccessibleTree(mParent);

      nsCOMPtr<nsIAccessible> logicalParent;
      treeAcc->GetTreeItemAccessible(parentIndex, getter_AddRefs(logicalParent));
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

  return DoCommand(nsnull, aIndex);
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

nsresult
nsXULTreeItemAccessibleBase::Shutdown()
{
  mTree = nsnull;
  mTreeView = nsnull;
  mRow = -1;

  return nsAccessibleWrap::Shutdown();
}




nsresult
nsXULTreeItemAccessibleBase::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);

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

  
  nsAccUtils::SetAccGroupAttrs(aAttributes, level + 1, posInSet, setSize);
  return NS_OK;
}

nsresult
nsXULTreeItemAccessibleBase::GetStateInternal(PRUint32 *aState,
                                              PRUint32 *aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);

  *aState = 0;
  if (aExtraState)
    *aExtraState = 0;

  if (IsDefunct()) {
    if (aExtraState)
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;
    return NS_OK_DEFUNCT_OBJECT;
  }

  
  *aState = nsIAccessibleStates::STATE_FOCUSABLE |
    nsIAccessibleStates::STATE_SELECTABLE;

  
  if (IsExpandable()) {
    PRBool isContainerOpen;
    mTreeView->IsContainerOpen(mRow, &isContainerOpen);
    *aState |= isContainerOpen ?
      nsIAccessibleStates::STATE_EXPANDED:
      nsIAccessibleStates::STATE_COLLAPSED;
  }

  
  nsCOMPtr<nsITreeSelection> selection;
  mTreeView->GetSelection(getter_AddRefs(selection));
  if (selection) {
    PRBool isSelected;
    selection->IsSelected(mRow, &isSelected);
    if (isSelected)
      *aState |= nsIAccessibleStates::STATE_SELECTED;
  }

  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
  do_QueryInterface(mDOMNode);
  if (multiSelect) {
    PRInt32 currentIndex;
    multiSelect->GetCurrentIndex(&currentIndex);
    if (currentIndex == mRow) {
      *aState |= nsIAccessibleStates::STATE_FOCUSED;
    }
  }

  
  PRInt32 firstVisibleRow, lastVisibleRow;
  mTree->GetFirstVisibleRow(&firstVisibleRow);
  mTree->GetLastVisibleRow(&lastVisibleRow);
  if (mRow < firstVisibleRow || mRow > lastVisibleRow)
    *aState |= nsIAccessibleStates::STATE_INVISIBLE;

  return NS_OK;
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
        if (!nsCoreUtils::IsColumnHidden(primaryColumn))
          return PR_TRUE;
      }
    }
  }

  return PR_FALSE;
}






nsXULTreeItemAccessible::
nsXULTreeItemAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                        nsIAccessible *aParent, nsITreeBoxObject *aTree,
                        nsITreeView *aTreeView, PRInt32 aRow) :
  nsXULTreeItemAccessibleBase(aDOMNode, aShell, aParent, aTree, aTreeView, aRow)
{
  mColumn = nsCoreUtils::GetFirstSensibleColumn(mTree);
}




NS_IMETHODIMP
nsXULTreeItemAccessible::GetFirstChild(nsIAccessible **aFirstChild)
{
  NS_ENSURE_ARG_POINTER(aFirstChild);
  *aFirstChild = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessible::GetLastChild(nsIAccessible **aLastChild)
{
  NS_ENSURE_ARG_POINTER(aLastChild);
  *aLastChild = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsXULTreeItemAccessible::GetChildCount(PRInt32 *aChildCount)
{
  NS_ENSURE_ARG_POINTER(aChildCount);
  *aChildCount = 0;

  return NS_OK;
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

nsresult
nsXULTreeItemAccessible::Init()
{
  nsresult rv = nsXULTreeItemAccessibleBase::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return GetName(mCachedName);
}

nsresult
nsXULTreeItemAccessible::Shutdown()
{
  mColumn = nsnull;
  return nsXULTreeItemAccessibleBase::Shutdown();
}




nsresult
nsXULTreeItemAccessible::GetRoleInternal(PRUint32 *aRole)
{
  nsCOMPtr<nsITreeColumn> column =
    nsCoreUtils::GetFirstSensibleColumn(mTree);

  PRBool isPrimary = PR_FALSE;
  column->GetPrimary(&isPrimary);

  *aRole = isPrimary ?
    nsIAccessibleRole::ROLE_OUTLINEITEM :
    nsIAccessibleRole::ROLE_LISTITEM;

  return NS_OK;
}




void
nsXULTreeItemAccessible::RowInvalidated(PRInt32 aStartColIdx,
                                        PRInt32 aEndColIdx)
{
  nsAutoString name;
  GetName(name);

  if (name != mCachedName) {
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE, this);
    mCachedName = name;
  }
}






nsXULTreeColumnsAccessible::
  nsXULTreeColumnsAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
  nsXULColumnsAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP
nsXULTreeColumnsAccessible::GetNextSibling(nsIAccessible **aNextSibling)
{
  NS_ENSURE_ARG_POINTER(aNextSibling);
  *aNextSibling = nsnull;

  nsCOMPtr<nsITreeBoxObject> tree;
  nsCOMPtr<nsITreeView> treeView;

  nsCoreUtils::GetTreeBoxObject(mDOMNode, getter_AddRefs(tree));
  if (tree) {
    tree->GetView(getter_AddRefs(treeView));
    if (treeView) {
      PRInt32 rowCount = 0;
      treeView->GetRowCount(&rowCount);
      if (rowCount > 0) {
        nsRefPtr<nsXULTreeAccessible> treeAcc =
          nsAccUtils::QueryAccessibleTree(mParent);
        NS_ENSURE_STATE(treeAcc);

        treeAcc->GetTreeItemAccessible(0, aNextSibling);
      }
    }
  }

  return NS_OK;
}

