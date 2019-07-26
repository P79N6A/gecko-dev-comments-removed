




#include <stdio.h>                      

#include "JoinElementTxn.h"
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsIContent.h"                 
#include "nsIDOMCharacterData.h"        
#include "nsIEditor.h"                  
#include "nsISupportsImpl.h"            

using namespace mozilla;

JoinElementTxn::JoinElementTxn()
  : EditTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(JoinElementTxn, EditTxn,
                                   mLeftNode,
                                   mRightNode,
                                   mParent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(JoinElementTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP JoinElementTxn::Init(nsEditor   *aEditor,
                                   nsINode *aLeftNode,
                                   nsINode *aRightNode)
{
  NS_PRECONDITION((aEditor && aLeftNode && aRightNode), "null arg");
  if (!aEditor || !aLeftNode || !aRightNode) { return NS_ERROR_NULL_POINTER; }
  mEditor = aEditor;
  mLeftNode = aLeftNode;
  nsCOMPtr<nsINode> leftParent = mLeftNode->GetParentNode();
  if (!mEditor->IsModifiableNode(leftParent)) {
    return NS_ERROR_FAILURE;
  }
  mRightNode = aRightNode;
  mOffset = 0;
  return NS_OK;
}


NS_IMETHODIMP JoinElementTxn::DoTransaction(void)
{
  NS_PRECONDITION((mEditor && mLeftNode && mRightNode), "null arg");
  if (!mEditor || !mLeftNode || !mRightNode) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsINode> leftParent = mLeftNode->GetParentNode();
  NS_ENSURE_TRUE(leftParent, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsINode> rightParent = mRightNode->GetParentNode();
  NS_ENSURE_TRUE(rightParent, NS_ERROR_NULL_POINTER);

  if (leftParent != rightParent) {
    NS_ASSERTION(false, "2 nodes do not have same parent");
    return NS_ERROR_INVALID_ARG;
  }

  
  
  mParent = leftParent;
  mOffset = mLeftNode->Length();

  return mEditor->JoinNodesImpl(mRightNode, mLeftNode, mParent);
}



NS_IMETHODIMP JoinElementTxn::UndoTransaction(void)
{
  NS_ASSERTION(mRightNode && mLeftNode && mParent, "bad state");
  if (!mRightNode || !mLeftNode || !mParent) { return NS_ERROR_NOT_INITIALIZED; }
  
  nsCOMPtr<nsIDOMCharacterData>rightNodeAsText = do_QueryInterface(mRightNode);
  ErrorResult rv;
  if (rightNodeAsText)
  {
    rv = rightNodeAsText->DeleteData(0, mOffset);
    NS_ENSURE_SUCCESS(rv.ErrorCode(), rv.ErrorCode());
  }
  else
  {
    for (nsCOMPtr<nsINode> child = mRightNode->GetFirstChild();
         child;
         child = child->GetNextSibling())
    {
      mLeftNode->AppendChild(*child, rv);
      NS_ENSURE_SUCCESS(rv.ErrorCode(), rv.ErrorCode());
    }
  }
  
  mParent->InsertBefore(*mLeftNode, mRightNode, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP JoinElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("JoinElementTxn");
  return NS_OK;
}
