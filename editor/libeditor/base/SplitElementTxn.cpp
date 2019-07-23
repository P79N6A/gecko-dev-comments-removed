




































#include "SplitElementTxn.h"
#include "nsEditor.h"
#include "nsIDOMNode.h"
#include "nsISelection.h"
#include "nsIDOMCharacterData.h"

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#endif



SplitElementTxn::SplitElementTxn()
  : EditTxn()
{
}

NS_IMETHODIMP SplitElementTxn::Init(nsEditor   *aEditor,
                                    nsIDOMNode *aNode,
                                    PRInt32     aOffset)
{
  NS_ASSERTION(aEditor && aNode, "bad args");
  if (!aEditor || !aNode) { return NS_ERROR_NOT_INITIALIZED; }

  mEditor = aEditor;
  mExistingRightNode = do_QueryInterface(aNode);
  mOffset = aOffset;
  return NS_OK;
}

NS_IMETHODIMP SplitElementTxn::DoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { printf("%p Do Split of node %p offset %d\n", this, mExistingRightNode.get(), mOffset); }
#endif

  NS_ASSERTION(mExistingRightNode && mEditor, "bad state");
  if (!mExistingRightNode || !mEditor) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsresult result = mExistingRightNode->CloneNode(PR_FALSE, getter_AddRefs(mNewLeftNode));
  NS_ASSERTION(((NS_SUCCEEDED(result)) && (mNewLeftNode)), "could not create element.");
  if (NS_FAILED(result)) return result;
  if (!mNewLeftNode) return NS_ERROR_NULL_POINTER;
  mEditor->MarkNodeDirty(mExistingRightNode);

#ifdef NS_DEBUG
  if (gNoisy) { printf("  created left node = %p\n", mNewLeftNode.get()); }
#endif

  
  result = mExistingRightNode->GetParentNode(getter_AddRefs(mParent));
  if (NS_FAILED(result)) return result;
  if (!mParent) return NS_ERROR_NULL_POINTER;

  
  result = mEditor->SplitNodeImpl(mExistingRightNode, mOffset, mNewLeftNode, mParent);
  if (NS_SUCCEEDED(result) && mNewLeftNode)
  {
    nsCOMPtr<nsISelection>selection;
    mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) return result;
    if (!selection) return NS_ERROR_NULL_POINTER;
    result = selection->Collapse(mNewLeftNode, mOffset);
  }
  else {
    result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

NS_IMETHODIMP SplitElementTxn::UndoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy) { 
    printf("%p Undo Split of existing node %p and new node %p offset %d\n", 
           this, mExistingRightNode.get(), mNewLeftNode.get(), mOffset); 
  }
#endif

  NS_ASSERTION(mEditor && mExistingRightNode && mNewLeftNode && mParent, "bad state");
  if (!mEditor || !mExistingRightNode || !mNewLeftNode || !mParent) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  nsresult result = mEditor->JoinNodesImpl(mExistingRightNode, mNewLeftNode, mParent, PR_FALSE);
#ifdef NS_DEBUG
  if (gNoisy) 
  { 
    printf("** after join left child node %p into right node %p\n", mNewLeftNode.get(), mExistingRightNode.get());
    if (gNoisy) {mEditor->DebugDumpContent(); } 
  }
  if (NS_SUCCEEDED(result))
  {
    if (gNoisy) { printf("  left node = %p removed\n", mNewLeftNode.get()); }
  }
#endif

  return result;
}




NS_IMETHODIMP SplitElementTxn::RedoTransaction(void)
{
  NS_ASSERTION(mEditor && mExistingRightNode && mNewLeftNode && mParent, "bad state");
  if (!mEditor || !mExistingRightNode || !mNewLeftNode || !mParent) {
    return NS_ERROR_NOT_INITIALIZED;
  }

#ifdef NS_DEBUG
  if (gNoisy) { 
    printf("%p Redo Split of existing node %p and new node %p offset %d\n", 
           this, mExistingRightNode.get(), mNewLeftNode.get(), mOffset); 
    if (gNoisy) {mEditor->DebugDumpContent(); } 
  }
#endif

  nsresult result;
  nsCOMPtr<nsIDOMNode>resultNode;
  
  nsCOMPtr<nsIDOMCharacterData>rightNodeAsText = do_QueryInterface(mExistingRightNode);
  if (rightNodeAsText)
  {
    result = rightNodeAsText->DeleteData(0, mOffset);
#ifdef NS_DEBUG
    if (gNoisy) 
    { 
      printf("** after delete of text in right text node %p offset %d\n", rightNodeAsText.get(), mOffset);
      mEditor->DebugDumpContent();  
    }
#endif
  }
  else
  {
    nsCOMPtr<nsIDOMNode>child;
    nsCOMPtr<nsIDOMNode>nextSibling;
    result = mExistingRightNode->GetFirstChild(getter_AddRefs(child));
    PRInt32 i;
    for (i=0; i<mOffset; i++)
    {
      if (NS_FAILED(result)) {return result;}
      if (!child) {return NS_ERROR_NULL_POINTER;}
      child->GetNextSibling(getter_AddRefs(nextSibling));
      result = mExistingRightNode->RemoveChild(child, getter_AddRefs(resultNode));
      if (NS_SUCCEEDED(result)) 
      {
        result = mNewLeftNode->AppendChild(child, getter_AddRefs(resultNode));
#ifdef NS_DEBUG
        if (gNoisy) 
        { 
          printf("** move child node %p from right node %p to left node %p\n", child.get(), mExistingRightNode.get(), mNewLeftNode.get());
          if (gNoisy) {mEditor->DebugDumpContent(); } 
        }
#endif
      }
      child = do_QueryInterface(nextSibling);
    }
  }
  
  result = mParent->InsertBefore(mNewLeftNode, mExistingRightNode, getter_AddRefs(resultNode));
#ifdef NS_DEBUG
  if (gNoisy) 
  { 
    printf("** reinsert left child node %p before right node %p\n", mNewLeftNode.get(), mExistingRightNode.get());
    if (gNoisy) {mEditor->DebugDumpContent(); } 
  }
#endif
  return result;
}


NS_IMETHODIMP SplitElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("SplitElementTxn");
  return NS_OK;
}

NS_IMETHODIMP SplitElementTxn::GetNewNode(nsIDOMNode **aNewNode)
{
  if (!aNewNode)
    return NS_ERROR_NULL_POINTER;
  if (!mNewLeftNode)
    return NS_ERROR_NOT_INITIALIZED;
  *aNewNode = mNewLeftNode;
  NS_ADDREF(*aNewNode);
  return NS_OK;
}
