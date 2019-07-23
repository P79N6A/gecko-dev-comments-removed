



































#include "nsASN1Tree.h"
#include "nsIComponentManager.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"

NS_IMPL_THREADSAFE_ISUPPORTS2(nsNSSASN1Tree, nsIASN1Tree, 
                                                 nsITreeView)

nsNSSASN1Tree::nsNSSASN1Tree() 
:mTopNode(nsnull)
{
}

nsNSSASN1Tree::~nsNSSASN1Tree()
{
  ClearNodes();
}

void nsNSSASN1Tree::ClearNodesRecursively(myNode *n)
{
  myNode *walk = n;
  while (walk) {
    myNode *kill = walk;

    if (walk->child) {
      ClearNodesRecursively(walk->child);
    }
    
    walk = walk->next;
    delete kill;
  }
}

void nsNSSASN1Tree::ClearNodes()
{
  ClearNodesRecursively(mTopNode);
  mTopNode = nsnull;
}

void nsNSSASN1Tree::InitChildsRecursively(myNode *n)
{
  if (!n->obj)
    return;

  n->seq = do_QueryInterface(n->obj);
  if (!n->seq)
    return;

  
  
  
  
  
  
  

  PRBool isContainer;
  n->seq->GetIsValidContainer(&isContainer);
  if (!isContainer) {
    n->seq = nsnull;
    return;
  }

  nsCOMPtr<nsIMutableArray> asn1Objects;
  n->seq->GetASN1Objects(getter_AddRefs(asn1Objects));
  PRUint32 numObjects;
  asn1Objects->GetLength(&numObjects);
  
  if (!numObjects) {
    n->seq = nsnull;
    return;
  }
  
  myNode *walk = nsnull;
  myNode *prev = nsnull;
  
  PRUint32 i;
  nsCOMPtr<nsISupports> isupports;
  for (i=0; i<numObjects; i++) {
    if (0 == i) {
      n->child = walk = new myNode;
    }
    else {
      walk = new myNode;
    }

    walk->parent = n;
    if (prev) {
      prev->next = walk;
    }
  
    walk->obj = do_QueryElementAt(asn1Objects, i);

    InitChildsRecursively(walk);

    prev = walk;
  }
}

void nsNSSASN1Tree::InitNodes()
{
  ClearNodes();

  mTopNode = new myNode;
  mTopNode->obj = mASN1Object;

  InitChildsRecursively(mTopNode);
}


NS_IMETHODIMP 
nsNSSASN1Tree::LoadASN1Structure(nsIASN1Object *asn1Object)
{
  
  
  
  
  
  
  PRBool redraw = (mASN1Object && mTree);
  PRInt32 rowsToDelete = 0;

  if (redraw) {
    
    
    rowsToDelete = 0-CountVisibleNodes(mTopNode);
  }

  mASN1Object = asn1Object;
  InitNodes();

  if (redraw) {
    
    PRInt32 newRows = CountVisibleNodes(mTopNode);
    mTree->BeginUpdateBatch();
    
    mTree->RowCountChanged(0, rowsToDelete);
    
    mTree->RowCountChanged(0, newRows);
    mTree->EndUpdateBatch();
  }

  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetRowCount(PRInt32 *aRowCount)
{
  if (mASN1Object) {
    *aRowCount = CountVisibleNodes(mTopNode);
  } else {
    *aRowCount = 0;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetSelection(nsITreeSelection * *aSelection)
{
  *aSelection = mSelection;
  NS_IF_ADDREF(*aSelection);
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSASN1Tree::SetSelection(nsITreeSelection * aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  return NS_OK;
}



NS_IMETHODIMP 
nsNSSASN1Tree::GetCellProperties(PRInt32 row, nsITreeColumn* col, 
                                 nsISupportsArray *properties)
{
  return NS_OK;
}



NS_IMETHODIMP 
nsNSSASN1Tree::GetColumnProperties(nsITreeColumn* col, 
                                   nsISupportsArray *properties)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::IsContainer(PRInt32 index, PRBool *_retval)
{
  myNode *n = FindNodeFromIndex(index);
  if (!n)
    return NS_ERROR_FAILURE;

  *_retval = (n->seq != nsnull);
  return NS_OK; 
}


NS_IMETHODIMP 
nsNSSASN1Tree::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
  myNode *n = FindNodeFromIndex(index);
  if (!n || !n->seq)
    return NS_ERROR_FAILURE;

  n->seq->GetIsExpanded(_retval);
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::IsContainerEmpty(PRInt32 index, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::IsSeparator(PRInt32 index, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK; 
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  PRInt32 parentIndex;
  PRInt32 nodeLevel;

  myNode *n = FindNodeFromIndex(index, &parentIndex, &nodeLevel);
  if (!n)
    return NS_ERROR_FAILURE;

  *_retval = nodeLevel;
  return NS_OK; 
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetImageSrc(PRInt32 row, nsITreeColumn* col, 
                           nsAString& _retval)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetProgressMode(PRInt32 row, nsITreeColumn* col, PRInt32* _retval)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetCellValue(PRInt32 row, nsITreeColumn* col, 
                            nsAString& _retval)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetCellText(PRInt32 row, nsITreeColumn* col, 
                           nsAString& _retval)
{
  _retval.Truncate();

  myNode* n = FindNodeFromIndex(row);
  if (!n)
    return NS_ERROR_FAILURE;

  
  return n->obj->GetDisplayName(_retval);
}


NS_IMETHODIMP 
nsNSSASN1Tree::GetDisplayData(PRUint32 index, nsAString &_retval)
{
  myNode *n = FindNodeFromIndex(index);
  if (!n)
    return NS_ERROR_FAILURE;

  n->obj->GetDisplayValue(_retval);
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::SetTree(nsITreeBoxObject *tree)
{
  mTree = tree;
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::ToggleOpenState(PRInt32 index)
{
  myNode *n = FindNodeFromIndex(index);
  if (!n)
    return NS_ERROR_FAILURE;

  if (!n->seq)
    return NS_ERROR_FAILURE;

  PRBool IsExpanded;
  n->seq->GetIsExpanded(&IsExpanded);
  PRInt32 rowCountChange;
  if (IsExpanded) {
    rowCountChange = -CountVisibleNodes(n->child);
    n->seq->SetIsExpanded(PR_FALSE);
  } else {
    n->seq->SetIsExpanded(PR_TRUE);
    rowCountChange = CountVisibleNodes(n->child);
  }
  if (mTree)
    mTree->RowCountChanged(index, rowCountChange);
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::CycleHeader(nsITreeColumn* col)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::SelectionChanged()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP 
nsNSSASN1Tree::CycleCell(PRInt32 row, nsITreeColumn* col)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::IsEditable(PRInt32 row, nsITreeColumn* col, 
                          PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::IsSelectable(PRInt32 row, nsITreeColumn* col, 
                            PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::SetCellValue(PRInt32 row, nsITreeColumn* col, 
                            const nsAString& value)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::SetCellText(PRInt32 row, nsITreeColumn* col, 
                           const nsAString& value)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::PerformAction(const PRUnichar *action)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSASN1Tree::PerformActionOnCell(const PRUnichar *action, PRInt32 row, 
                                   nsITreeColumn* col)
{
  return NS_OK;
}




NS_IMETHODIMP nsNSSASN1Tree::CanDrop(PRInt32 index, PRInt32 orientation,
                                     nsIDOMDataTransfer* aDataTransfer, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  
  return NS_OK;
}





NS_IMETHODIMP nsNSSASN1Tree::Drop(PRInt32 row, PRInt32 orient, nsIDOMDataTransfer* aDataTransfer)
{
  return NS_OK;
}







NS_IMETHODIMP nsNSSASN1Tree::IsSorted(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}



NS_IMETHODIMP 
nsNSSASN1Tree::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  PRInt32 parentIndex = -1;

  myNode *n = FindNodeFromIndex(rowIndex, &parentIndex);
  if (!n)
    return NS_ERROR_FAILURE;

  *_retval = parentIndex;
  return NS_OK; 
}


NS_IMETHODIMP 
nsNSSASN1Tree::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, 
                              PRBool *_retval)
{
  myNode *n = FindNodeFromIndex(rowIndex);
  if (!n)
    return NS_ERROR_FAILURE;

  if (!n->next) {
    *_retval = PR_FALSE;
  }
  else {
    PRInt32 nTotalSize = CountVisibleNodes(n);
    PRInt32 nLastChildPos = rowIndex + nTotalSize -1;
    PRInt32 nextSiblingPos = nLastChildPos +1;
    *_retval = (nextSiblingPos > afterIndex);
  }

  return NS_OK; 
}

PRInt32 nsNSSASN1Tree::CountVisibleNodes(myNode *n)
{
  if (!n)
    return 0;

  myNode *walk = n;
  PRInt32 count = 0;
  
  while (walk) {
    ++count;

    if (walk->seq) {
      PRBool IsExpanded;
      walk->seq->GetIsExpanded(&IsExpanded);
      if (IsExpanded) {
        count += CountVisibleNodes(walk->child);
      }
    }

    walk = walk->next;
  }

  return count;
}


nsNSSASN1Tree::myNode *
nsNSSASN1Tree::FindNodeFromIndex(PRInt32 wantedIndex, 
                                 PRInt32 *optionalOutParentIndex, PRInt32 *optionalOutLevel)
{
  if (0 == wantedIndex) {
    if (optionalOutLevel) {
      *optionalOutLevel = 0;
    }
    if (optionalOutParentIndex) {
      *optionalOutParentIndex = -1;
    }
    return mTopNode;
  }
  else {
    PRInt32 index = 0;
    PRInt32 level = 0;
    return FindNodeFromIndex(mTopNode, wantedIndex, index, level, 
                             optionalOutParentIndex, optionalOutLevel);
  }
}


nsNSSASN1Tree::myNode *
nsNSSASN1Tree::FindNodeFromIndex(myNode *n, PRInt32 wantedIndex,
                                 PRInt32 &index_counter, PRInt32 &level_counter,
                                 PRInt32 *optionalOutParentIndex, PRInt32 *optionalOutLevel)
{
  if (!n)
    return nsnull;

  myNode *walk = n;
  PRInt32 parentIndex = index_counter-1;
  
  while (walk) {
    if (index_counter == wantedIndex) {
      if (optionalOutLevel) {
        *optionalOutLevel = level_counter;
      }
      if (optionalOutParentIndex) {
        *optionalOutParentIndex = parentIndex;
      }
      return walk;
    }

    if (walk->seq) {
      PRBool IsExpanded;
      walk->seq->GetIsExpanded(&IsExpanded);
      if (IsExpanded) {
        ++index_counter; 

        ++level_counter;
        myNode *found = FindNodeFromIndex(walk->child, wantedIndex, index_counter, level_counter,
                                          optionalOutParentIndex, optionalOutLevel);
        --level_counter;

        if (found)
          return found;
      }
    }

    walk = walk->next;
    if (walk) {
      ++index_counter;
    }
  }

  return nsnull;
}

