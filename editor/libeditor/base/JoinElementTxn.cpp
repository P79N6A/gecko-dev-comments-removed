




































#include "JoinElementTxn.h"
#include "nsEditor.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMCharacterData.h"

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#endif

JoinElementTxn::JoinElementTxn()
  : EditTxn()
{
}

NS_IMETHODIMP JoinElementTxn::Init(nsEditor   *aEditor,
                                   nsIDOMNode *aLeftNode,
                                   nsIDOMNode *aRightNode)
{
  NS_PRECONDITION((aEditor && aLeftNode && aRightNode), "null arg");
  if (!aEditor || !aLeftNode || !aRightNode) { return NS_ERROR_NULL_POINTER; }
  mEditor = aEditor;
  mLeftNode = do_QueryInterface(aLeftNode);
  mRightNode = do_QueryInterface(aRightNode);
  mOffset=0;
  return NS_OK;
}


NS_IMETHODIMP JoinElementTxn::DoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("%p Do Join of %p and %p\n", this, mLeftNode.get(), mRightNode.get()); }
#endif

  NS_PRECONDITION((mEditor && mLeftNode && mRightNode), "null arg");
  if (!mEditor || !mLeftNode || !mRightNode) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIDOMNode>leftParent;
  nsresult result = mLeftNode->GetParentNode(getter_AddRefs(leftParent));
  if (NS_FAILED(result)) return result;
  if (!leftParent) return NS_ERROR_NULL_POINTER;

  
  nsCOMPtr<nsIDOMNode>rightParent;
  result = mRightNode->GetParentNode(getter_AddRefs(rightParent));
  if (NS_FAILED(result)) return result;
  if (!rightParent) return NS_ERROR_NULL_POINTER;

  if (leftParent==rightParent)
  {
    mParent= do_QueryInterface(leftParent); 
                                            
    nsCOMPtr<nsIDOMCharacterData> leftNodeAsText = do_QueryInterface(mLeftNode);
    if (leftNodeAsText) 
    {
      leftNodeAsText->GetLength(&mOffset);
    }
    else 
    {
      nsCOMPtr<nsIDOMNodeList> childNodes;
      result = mLeftNode->GetChildNodes(getter_AddRefs(childNodes));
      if (NS_FAILED(result)) return result;
      if (childNodes) 
      {
        childNodes->GetLength(&mOffset);
      }
    }
    result = mEditor->JoinNodesImpl(mRightNode, mLeftNode, mParent, PR_FALSE);
#ifdef NS_DEBUG
    if (NS_SUCCEEDED(result))
    {
      if (gNoisy) { printf("  left node = %p removed\n", mLeftNode.get()); }
    }
#endif
  }
  else 
  {
    NS_ASSERTION(PR_FALSE, "2 nodes do not have same parent");
    return NS_ERROR_INVALID_ARG;
  }
  return result;
}



NS_IMETHODIMP JoinElementTxn::UndoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("%p Undo Join, right node = %p\n", this, mRightNode.get()); }
#endif

  NS_ASSERTION(mRightNode && mLeftNode && mParent, "bad state");
  if (!mRightNode || !mLeftNode || !mParent) { return NS_ERROR_NOT_INITIALIZED; }
  nsresult result;
  nsCOMPtr<nsIDOMNode>resultNode;
  
  nsCOMPtr<nsIDOMCharacterData>rightNodeAsText = do_QueryInterface(mRightNode);
  if (rightNodeAsText)
  {
    result = rightNodeAsText->DeleteData(0, mOffset);
  }
  else
  {
    nsCOMPtr<nsIDOMNode>child;
    result = mRightNode->GetFirstChild(getter_AddRefs(child));
    nsCOMPtr<nsIDOMNode>nextSibling;
    PRUint32 i;
    for (i=0; i<mOffset; i++)
    {
      if (NS_FAILED(result)) {return result;}
      if (!child) {return NS_ERROR_NULL_POINTER;}
      child->GetNextSibling(getter_AddRefs(nextSibling));
      result = mLeftNode->AppendChild(child, getter_AddRefs(resultNode));
      child = do_QueryInterface(nextSibling);
    }
  }
  
  result = mParent->InsertBefore(mLeftNode, mRightNode, getter_AddRefs(resultNode));
  return result;

}

NS_IMETHODIMP JoinElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("JoinElementTxn");
  return NS_OK;
}
