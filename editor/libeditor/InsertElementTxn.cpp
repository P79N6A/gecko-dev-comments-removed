




#include <stdio.h>                      

#include "InsertElementTxn.h"
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsIContent.h"                 
#include "nsINode.h"                    
#include "nsISelection.h"               
#include "nsMemory.h"                   
#include "nsReadableUtils.h"            
#include "nsString.h"                   

using namespace mozilla;

InsertElementTxn::InsertElementTxn()
  : EditTxn()
{
}

InsertElementTxn::~InsertElementTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(InsertElementTxn, EditTxn,
                                   mNode,
                                   mParent)

NS_IMPL_ADDREF_INHERITED(InsertElementTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(InsertElementTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(InsertElementTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP InsertElementTxn::Init(nsIDOMNode *aNode,
                                     nsIDOMNode *aParent,
                                     int32_t     aOffset,
                                     nsIEditor  *aEditor)
{
  NS_ASSERTION(aNode && aParent && aEditor, "bad arg");
  NS_ENSURE_TRUE(aNode && aParent && aEditor, NS_ERROR_NULL_POINTER);

  mNode = do_QueryInterface(aNode);
  mParent = do_QueryInterface(aParent);
  mOffset = aOffset;
  mEditor = aEditor;
  NS_ENSURE_TRUE(mNode && mParent && mEditor, NS_ERROR_INVALID_ARG);
  return NS_OK;
}


NS_IMETHODIMP InsertElementTxn::DoTransaction(void)
{
  NS_ENSURE_TRUE(mNode && mParent, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsINode> parent = do_QueryInterface(mParent);
  NS_ENSURE_STATE(parent);

  uint32_t count = parent->GetChildCount();
  if (mOffset > int32_t(count) || mOffset == -1) {
    
    mOffset = count;
  }

  
  nsCOMPtr<nsIContent> refContent = parent->GetChildAt(mOffset);
  nsCOMPtr<nsIDOMNode> refNode = refContent ? refContent->AsDOMNode() : nullptr;

  mEditor->MarkNodeDirty(mNode);

  nsCOMPtr<nsIDOMNode> resultNode;
  nsresult result = mParent->InsertBefore(mNode, refNode, getter_AddRefs(resultNode));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(resultNode, NS_ERROR_NULL_POINTER);

  
  bool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsISelection> selection;
    result = mEditor->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(result, result);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    
    selection->Collapse(mParent, mOffset+1);
  }
  else
  {
    
  }
  return result;
}

NS_IMETHODIMP InsertElementTxn::UndoTransaction(void)
{
  NS_ENSURE_TRUE(mNode && mParent, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIDOMNode> resultNode;
  return mParent->RemoveChild(mNode, getter_AddRefs(resultNode));
}

NS_IMETHODIMP InsertElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("InsertElementTxn");
  return NS_OK;
}
