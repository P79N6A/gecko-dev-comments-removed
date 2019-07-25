




#include "DeleteElementTxn.h"
#include "nsSelectionState.h" 
#include "nsEditor.h"


DeleteElementTxn::DeleteElementTxn()
  : EditTxn(), mNode(), mParent(), mRefNode(), mRangeUpdater(nsnull)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(DeleteElementTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DeleteElementTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRefNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DeleteElementTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRefNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(DeleteElementTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(DeleteElementTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DeleteElementTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

nsresult
DeleteElementTxn::Init(nsEditor* aEditor, nsINode* aNode,
                       nsRangeUpdater* aRangeUpdater)
{
  NS_ENSURE_TRUE(aEditor && aNode, NS_ERROR_NULL_POINTER);
  mEditor = aEditor;
  mNode = aNode;
  mParent = aNode->GetNodeParent();

  
  NS_ENSURE_TRUE(!mParent || mEditor->IsModifiableNode(mParent),
                 NS_ERROR_FAILURE);

  mRangeUpdater = aRangeUpdater;
  return NS_OK;
}


NS_IMETHODIMP
DeleteElementTxn::DoTransaction()
{
  NS_ENSURE_TRUE(mNode, NS_ERROR_NOT_INITIALIZED);

  if (!mParent) {
    
    return NS_OK;
  }

  
  
  mRefNode = mNode->GetNextSibling();

  
  
  
  if (mRangeUpdater) {
    mRangeUpdater->SelAdjDeleteNode(mNode->AsDOMNode());
  }

  return mParent->RemoveChild(mNode);
}

NS_IMETHODIMP
DeleteElementTxn::UndoTransaction()
{
  if (!mParent) {
    
    return NS_OK;
  }
  if (!mNode) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult res;
  mParent->InsertBefore(mNode, mRefNode, &res);
  return res;
}

NS_IMETHODIMP
DeleteElementTxn::RedoTransaction()
{
  if (!mParent) {
    
    return NS_OK;
  }
  if (!mNode) {
    return NS_ERROR_NULL_POINTER;
  }

  if (mRangeUpdater) {
    mRangeUpdater->SelAdjDeleteNode(mNode->AsDOMNode());
  }

  return mParent->RemoveChild(mNode);
}

NS_IMETHODIMP
DeleteElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("DeleteElementTxn");
  return NS_OK;
}
