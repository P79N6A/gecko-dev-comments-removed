




#include "JoinNodeTxn.h"
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsIContent.h"                 
#include "nsIDOMCharacterData.h"        
#include "nsIEditor.h"                  
#include "nsISupportsImpl.h"            

using namespace mozilla;
using namespace mozilla::dom;

JoinNodeTxn::JoinNodeTxn(nsEditor& aEditor, nsINode& aLeftNode,
                         nsINode& aRightNode)
  : EditTxn()
  , mEditor(aEditor)
  , mLeftNode(&aLeftNode)
  , mRightNode(&aRightNode)
  , mOffset(0)
  , mParent(nullptr)
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(JoinNodeTxn, EditTxn,
                                   mLeftNode,
                                   mRightNode,
                                   mParent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(JoinNodeTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

nsresult
JoinNodeTxn::CheckValidity()
{
  if (!mEditor.IsModifiableNode(mLeftNode->GetParentNode())) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}



NS_IMETHODIMP
JoinNodeTxn::DoTransaction()
{
  
  nsCOMPtr<nsINode> leftParent = mLeftNode->GetParentNode();
  NS_ENSURE_TRUE(leftParent, NS_ERROR_NULL_POINTER);

  
  if (leftParent != mRightNode->GetParentNode()) {
    NS_ASSERTION(false, "Nodes do not have same parent");
    return NS_ERROR_INVALID_ARG;
  }

  
  
  mParent = leftParent;
  mOffset = mLeftNode->Length();

  return mEditor.JoinNodesImpl(mRightNode, mLeftNode, mParent);
}



NS_IMETHODIMP
JoinNodeTxn::UndoTransaction()
{
  MOZ_ASSERT(mParent);

  
  ErrorResult rv;
  if (mRightNode->GetAsText()) {
    rv = mRightNode->GetAsText()->DeleteData(0, mOffset);
  } else {
    nsCOMPtr<nsIContent> child = mRightNode->GetFirstChild();
    for (uint32_t i = 0; i < mOffset; i++) {
      if (rv.Failed()) {
        return rv.StealNSResult();
      }
      if (!child) {
        return NS_ERROR_NULL_POINTER;
      }
      nsCOMPtr<nsIContent> nextSibling = child->GetNextSibling();
      mLeftNode->AppendChild(*child, rv);
      child = nextSibling;
    }
  }
  
  mParent->InsertBefore(*mLeftNode, mRightNode, rv);
  return rv.StealNSResult();
}

NS_IMETHODIMP
JoinNodeTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("JoinNodeTxn");
  return NS_OK;
}
