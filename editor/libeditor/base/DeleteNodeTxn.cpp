




#include "DeleteNodeTxn.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsError.h"
#include "nsSelectionState.h" 
#include "nsAString.h"


DeleteNodeTxn::DeleteNodeTxn()
  : EditTxn(), mNode(), mParent(), mRefNode(), mRangeUpdater(nsnull)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(DeleteNodeTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DeleteNodeTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRefNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DeleteNodeTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRefNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(DeleteNodeTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(DeleteNodeTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DeleteNodeTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

nsresult
DeleteNodeTxn::Init(nsEditor* aEditor, nsINode* aNode,
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
DeleteNodeTxn::DoTransaction()
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
DeleteNodeTxn::UndoTransaction()
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
DeleteNodeTxn::RedoTransaction()
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
DeleteNodeTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("DeleteNodeTxn");
  return NS_OK;
}
