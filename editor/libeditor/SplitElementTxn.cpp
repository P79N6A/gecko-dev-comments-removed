




#include <stdio.h>                      

#include "SplitElementTxn.h"
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsIContent.h"                 
#include "nsIDOMCharacterData.h"        
#include "nsIEditor.h"                  
#include "nsISelection.h"               
#include "nsISupportsUtils.h"           

using namespace mozilla;


SplitElementTxn::SplitElementTxn()
  : EditTxn()
{
}

SplitElementTxn::~SplitElementTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(SplitElementTxn, EditTxn,
                                   mParent,
                                   mNewLeftNode)

NS_IMPL_ADDREF_INHERITED(SplitElementTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(SplitElementTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SplitElementTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP SplitElementTxn::Init(nsEditor   *aEditor,
                                    nsIDOMNode *aNode,
                                    int32_t     aOffset)
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
  NS_ASSERTION(mExistingRightNode && mEditor, "bad state");
  if (!mExistingRightNode || !mEditor) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsresult result = mExistingRightNode->CloneNode(false, 1, getter_AddRefs(mNewLeftNode));
  NS_ASSERTION(((NS_SUCCEEDED(result)) && (mNewLeftNode)), "could not create element.");
  NS_ENSURE_SUCCESS(result, result);
  mEditor->MarkNodeDirty(mExistingRightNode);

  
  result = mExistingRightNode->GetParentNode(getter_AddRefs(mParent));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(mParent, NS_ERROR_NULL_POINTER);

  
  result = mEditor->SplitNodeImpl(mExistingRightNode, mOffset, mNewLeftNode, mParent);
  if (mNewLeftNode) {
    bool bAdjustSelection;
    mEditor->ShouldTxnSetSelection(&bAdjustSelection);
    if (bAdjustSelection)
    {
      nsCOMPtr<nsISelection> selection;
      result = mEditor->GetSelection(getter_AddRefs(selection));
      NS_ENSURE_SUCCESS(result, result);
      NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
      result = selection->Collapse(mNewLeftNode, mOffset);
    }
    else
    {
      
    }
  }
  return result;
}

NS_IMETHODIMP SplitElementTxn::UndoTransaction(void)
{
  NS_ASSERTION(mEditor && mExistingRightNode && mNewLeftNode && mParent, "bad state");
  if (!mEditor || !mExistingRightNode || !mNewLeftNode || !mParent) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  nsCOMPtr<nsINode> right = do_QueryInterface(mExistingRightNode);
  nsCOMPtr<nsINode> left = do_QueryInterface(mNewLeftNode);
  nsCOMPtr<nsINode> parent = do_QueryInterface(mParent);
  return mEditor->JoinNodesImpl(right, left, parent);
}




NS_IMETHODIMP SplitElementTxn::RedoTransaction(void)
{
  NS_ASSERTION(mEditor && mExistingRightNode && mNewLeftNode && mParent, "bad state");
  if (!mEditor || !mExistingRightNode || !mNewLeftNode || !mParent) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult result;
  nsCOMPtr<nsIDOMNode>resultNode;
  
  nsCOMPtr<nsIDOMCharacterData>rightNodeAsText = do_QueryInterface(mExistingRightNode);
  if (rightNodeAsText)
  {
    nsresult result = rightNodeAsText->DeleteData(0, mOffset);
    NS_ENSURE_SUCCESS(result, result);
  }
  else
  {
    nsCOMPtr<nsIDOMNode>child;
    nsCOMPtr<nsIDOMNode>nextSibling;
    result = mExistingRightNode->GetFirstChild(getter_AddRefs(child));
    int32_t i;
    for (i=0; i<mOffset; i++)
    {
      if (NS_FAILED(result)) {return result;}
      if (!child) {return NS_ERROR_NULL_POINTER;}
      child->GetNextSibling(getter_AddRefs(nextSibling));
      result = mExistingRightNode->RemoveChild(child, getter_AddRefs(resultNode));
      if (NS_SUCCEEDED(result))
      {
        result = mNewLeftNode->AppendChild(child, getter_AddRefs(resultNode));
      }
      child = do_QueryInterface(nextSibling);
    }
  }
  
  result = mParent->InsertBefore(mNewLeftNode, mExistingRightNode, getter_AddRefs(resultNode));
  return result;
}


NS_IMETHODIMP SplitElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("SplitElementTxn");
  return NS_OK;
}

NS_IMETHODIMP SplitElementTxn::GetNewNode(nsIDOMNode **aNewNode)
{
  NS_ENSURE_TRUE(aNewNode, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(mNewLeftNode, NS_ERROR_NOT_INITIALIZED);
  *aNewNode = mNewLeftNode;
  NS_ADDREF(*aNewNode);
  return NS_OK;
}
