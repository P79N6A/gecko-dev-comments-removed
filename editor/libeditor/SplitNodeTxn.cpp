




#include "SplitNodeTxn.h"

#include "mozilla/dom/Selection.h"
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsIContent.h"                 

using namespace mozilla;
using namespace mozilla::dom;


SplitNodeTxn::SplitNodeTxn(nsEditor& aEditor, nsIContent& aNode,
                           int32_t aOffset)
  : EditTxn()
  , mEditor(aEditor)
  , mExistingRightNode(&aNode)
  , mOffset(aOffset)
  , mNewLeftNode(nullptr)
  , mParent(nullptr)
{
}

SplitNodeTxn::~SplitNodeTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(SplitNodeTxn, EditTxn,
                                   mParent,
                                   mNewLeftNode)

NS_IMPL_ADDREF_INHERITED(SplitNodeTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(SplitNodeTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SplitNodeTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP
SplitNodeTxn::DoTransaction()
{
  
  ErrorResult rv;
  
  nsCOMPtr<nsINode> clone = mExistingRightNode->CloneNode(false, rv);
  NS_ASSERTION(!rv.Failed() && clone, "Could not create clone");
  NS_ENSURE_TRUE(!rv.Failed() && clone, rv.StealNSResult());
  mNewLeftNode = dont_AddRef(clone.forget().take()->AsContent());
  mEditor.MarkNodeDirty(mExistingRightNode->AsDOMNode());

  
  mParent = mExistingRightNode->GetParentNode();
  NS_ENSURE_TRUE(mParent, NS_ERROR_NULL_POINTER);

  
  rv = mEditor.SplitNodeImpl(*mExistingRightNode, mOffset, *mNewLeftNode);
  if (mEditor.GetShouldTxnSetSelection()) {
    nsRefPtr<Selection> selection = mEditor.GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    rv = selection->Collapse(mNewLeftNode, mOffset);
  }
  return rv.StealNSResult();
}

NS_IMETHODIMP
SplitNodeTxn::UndoTransaction()
{
  MOZ_ASSERT(mNewLeftNode && mParent);

  
  return mEditor.JoinNodesImpl(mExistingRightNode, mNewLeftNode, mParent);
}





NS_IMETHODIMP
SplitNodeTxn::RedoTransaction()
{
  MOZ_ASSERT(mNewLeftNode && mParent);

  ErrorResult rv;
  
  if (mExistingRightNode->GetAsText()) {
    rv = mExistingRightNode->GetAsText()->DeleteData(0, mOffset);
    NS_ENSURE_TRUE(!rv.Failed(), rv.StealNSResult());
  } else {
    nsCOMPtr<nsIContent> child = mExistingRightNode->GetFirstChild();
    nsCOMPtr<nsIContent> nextSibling;
    for (int32_t i=0; i < mOffset; i++) {
      if (rv.Failed()) {
        return rv.StealNSResult();
      }
      if (!child) {
        return NS_ERROR_NULL_POINTER;
      }
      nextSibling = child->GetNextSibling();
      mExistingRightNode->RemoveChild(*child, rv);
      if (!rv.Failed()) {
        mNewLeftNode->AppendChild(*child, rv);
      }
      child = nextSibling;
    }
  }
  
  mParent->InsertBefore(*mNewLeftNode, mExistingRightNode, rv);
  return rv.StealNSResult();
}


NS_IMETHODIMP
SplitNodeTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("SplitNodeTxn");
  return NS_OK;
}

nsIContent*
SplitNodeTxn::GetNewNode()
{
  return mNewLeftNode;
}
