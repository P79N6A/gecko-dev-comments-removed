






































#include "nsDocShellEnumerator.h"

#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"

nsDocShellEnumerator::nsDocShellEnumerator(PRInt32 inEnumerationDirection)
: mRootItem(nsnull)
, mCurIndex(0)
, mDocShellType(nsIDocShellTreeItem::typeAll)
, mArrayValid(PR_FALSE)
, mEnumerationDirection(inEnumerationDirection)
{
}

nsDocShellEnumerator::~nsDocShellEnumerator()
{
}

NS_IMPL_ISUPPORTS1(nsDocShellEnumerator, nsISimpleEnumerator)



NS_IMETHODIMP nsDocShellEnumerator::GetNext(nsISupports **outCurItem)
{
  NS_ENSURE_ARG_POINTER(outCurItem);
  *outCurItem = nsnull;
  
  nsresult rv = EnsureDocShellArray();
  if (NS_FAILED(rv)) return rv;
  
  if (mCurIndex >= mItemArray.Length()) {
    return NS_ERROR_FAILURE;
  }
  
  
  return CallQueryInterface(mItemArray[mCurIndex++], outCurItem);
}


NS_IMETHODIMP nsDocShellEnumerator::HasMoreElements(PRBool *outHasMore)
{
  NS_ENSURE_ARG_POINTER(outHasMore);
  *outHasMore = PR_FALSE;

  nsresult rv = EnsureDocShellArray();
  if (NS_FAILED(rv)) return rv;

  *outHasMore = (mCurIndex < mItemArray.Length());
  return NS_OK;
}

nsresult nsDocShellEnumerator::GetEnumerationRootItem(nsIDocShellTreeItem * *aEnumerationRootItem)
{
  NS_ENSURE_ARG_POINTER(aEnumerationRootItem);
  *aEnumerationRootItem = mRootItem;
  NS_IF_ADDREF(*aEnumerationRootItem);
  return NS_OK;
}

nsresult nsDocShellEnumerator::SetEnumerationRootItem(nsIDocShellTreeItem * aEnumerationRootItem)
{
  mRootItem = aEnumerationRootItem;
  ClearState();
  return NS_OK;
}

nsresult nsDocShellEnumerator::GetEnumDocShellType(PRInt32 *aEnumerationItemType)
{
  NS_ENSURE_ARG_POINTER(aEnumerationItemType);
  *aEnumerationItemType = mDocShellType;
  return NS_OK;
}

nsresult nsDocShellEnumerator::SetEnumDocShellType(PRInt32 aEnumerationItemType)
{
  mDocShellType = aEnumerationItemType;
  ClearState();
  return NS_OK;
}

nsresult nsDocShellEnumerator::First()
{
  mCurIndex = 0;
  return EnsureDocShellArray();
}

nsresult nsDocShellEnumerator::EnsureDocShellArray()
{
  if (!mArrayValid)
  {
    mArrayValid = PR_TRUE;
    return BuildDocShellArray(mItemArray);
  }
  
  return NS_OK;
}

nsresult nsDocShellEnumerator::ClearState()
{
  mItemArray.Clear();
  mArrayValid = PR_FALSE;
  mCurIndex = 0;
  return NS_OK;
}

nsresult nsDocShellEnumerator::BuildDocShellArray(nsTArray<nsIDocShellTreeItem*>& inItemArray)
{
  NS_ENSURE_TRUE(mRootItem, NS_ERROR_NOT_INITIALIZED);
  inItemArray.Clear();
  return BuildArrayRecursive(mRootItem, inItemArray);
}

nsresult nsDocShellForwardsEnumerator::BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsTArray<nsIDocShellTreeItem*>& inItemArray)
{
  nsresult rv;
  nsCOMPtr<nsIDocShellTreeNode> itemAsNode = do_QueryInterface(inItem, &rv);
  if (NS_FAILED(rv)) return rv;

  PRInt32   itemType;
  
  if ((mDocShellType == nsIDocShellTreeItem::typeAll) ||
      (NS_SUCCEEDED(inItem->GetItemType(&itemType)) && (itemType == mDocShellType)))
  {
    if (!inItemArray.AppendElement(inItem))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  PRInt32   numChildren;
  rv = itemAsNode->GetChildCount(&numChildren);
  if (NS_FAILED(rv)) return rv;
  
  for (PRInt32 i = 0; i < numChildren; ++i)
  {
    nsCOMPtr<nsIDocShellTreeItem> curChild;
    rv = itemAsNode->GetChildAt(i, getter_AddRefs(curChild));
    if (NS_FAILED(rv)) return rv;
      
    rv = BuildArrayRecursive(curChild, inItemArray);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}


nsresult nsDocShellBackwardsEnumerator::BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsTArray<nsIDocShellTreeItem*>& inItemArray)
{
  nsresult rv;
  nsCOMPtr<nsIDocShellTreeNode> itemAsNode = do_QueryInterface(inItem, &rv);
  if (NS_FAILED(rv)) return rv;

  PRInt32   numChildren;
  rv = itemAsNode->GetChildCount(&numChildren);
  if (NS_FAILED(rv)) return rv;
  
  for (PRInt32 i = numChildren - 1; i >= 0; --i)
  {
    nsCOMPtr<nsIDocShellTreeItem> curChild;
    rv = itemAsNode->GetChildAt(i, getter_AddRefs(curChild));
    if (NS_FAILED(rv)) return rv;
      
    rv = BuildArrayRecursive(curChild, inItemArray);
    if (NS_FAILED(rv)) return rv;
  }

  PRInt32   itemType;
  
  if ((mDocShellType == nsIDocShellTreeItem::typeAll) ||
      (NS_SUCCEEDED(inItem->GetItemType(&itemType)) && (itemType == mDocShellType)))
  {
    if (!inItemArray.AppendElement(inItem))
      return NS_ERROR_OUT_OF_MEMORY;
  }


  return NS_OK;
}


