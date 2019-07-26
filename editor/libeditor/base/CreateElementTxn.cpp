




#include <stdio.h>

#include "CreateElementTxn.h"
#include "mozilla/dom/Element.h"
#include "nsAlgorithm.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsError.h"
#include "nsIContent.h"
#include "nsIDOMCharacterData.h"
#include "nsINode.h"
#include "nsISelection.h"
#include "nsISupportsUtils.h"
#include "nsMemory.h"
#include "nsReadableUtils.h"
#include "nsStringFwd.h"
#include "nsString.h"
#include "nsAString.h"

#ifdef DEBUG
static bool gNoisy = false;
#endif

using namespace mozilla;

CreateElementTxn::CreateElementTxn()
  : EditTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(CreateElementTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(CreateElementTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNewNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRefNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(CreateElementTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNewNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRefNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(CreateElementTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(CreateElementTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CreateElementTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)
NS_IMETHODIMP CreateElementTxn::Init(nsEditor      *aEditor,
                                     const nsAString &aTag,
                                     nsINode       *aParent,
                                     uint32_t        aOffsetInParent)
{
  NS_ASSERTION(aEditor&&aParent, "null args");
  if (!aEditor || !aParent) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mTag = aTag;
  mParent = do_QueryInterface(aParent);
  mOffsetInParent = aOffsetInParent;
  return NS_OK;
}


NS_IMETHODIMP CreateElementTxn::DoTransaction(void)
{
#ifdef DEBUG
  if (gNoisy)
  {
    char* nodename = ToNewCString(mTag);
    printf("Do Create Element parent = %p <%s>, offset = %d\n", 
           static_cast<void*>(mParent.get()), nodename, mOffsetInParent);
    nsMemory::Free(nodename);
  }
#endif

  NS_ASSERTION(mEditor && mParent, "bad state");
  NS_ENSURE_TRUE(mEditor && mParent, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<dom::Element> newContent;

  
  nsresult result = mEditor->CreateHTMLContent(mTag, getter_AddRefs(newContent));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_STATE(newContent);

  mNewNode = newContent;
  
  mEditor->MarkNodeDirty(mNewNode);

#ifdef DEBUG
  if (gNoisy)
  {
    printf("  newNode = %p\n", static_cast<void*>(mNewNode.get()));
  }
#endif

  
  if (CreateElementTxn::eAppend == int32_t(mOffsetInParent)) {
    ErrorResult rv;
    mParent->AppendChild(*mNewNode, rv);
    return rv.ErrorCode();
  }


  mOffsetInParent = NS_MIN(mOffsetInParent, mParent->GetChildCount());

  
  mRefNode = mParent->GetChildAt(mOffsetInParent);

  ErrorResult rv;
  mParent->InsertBefore(*mNewNode, mRefNode, rv);
  NS_ENSURE_SUCCESS(rv.ErrorCode(), rv.ErrorCode());

  
  bool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (!bAdjustSelection) {
    
    return NS_OK;
  }

  nsCOMPtr<nsISelection> selection;
  result = mEditor->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContent> parentContent = do_QueryInterface(mParent);
  NS_ENSURE_STATE(parentContent);

  result = selection->CollapseNative(parentContent,
                                     parentContent->IndexOf(newContent) + 1);
  NS_ASSERTION((NS_SUCCEEDED(result)), "selection could not be collapsed after insert.");
  return result;
}

NS_IMETHODIMP CreateElementTxn::UndoTransaction(void)
{
#ifdef DEBUG
  if (gNoisy)
  {
    printf("Undo Create Element, mParent = %p, node = %p\n",
           static_cast<void*>(mParent.get()),
           static_cast<void*>(mNewNode.get()));
  }
#endif

  NS_ASSERTION(mEditor && mParent, "bad state");
  NS_ENSURE_TRUE(mEditor && mParent, NS_ERROR_NOT_INITIALIZED);

  ErrorResult rv;
  mParent->RemoveChild(*mNewNode, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP CreateElementTxn::RedoTransaction(void)
{
#ifdef DEBUG
  if (gNoisy) { printf("Redo Create Element\n"); }
#endif

  NS_ASSERTION(mEditor && mParent, "bad state");
  NS_ENSURE_TRUE(mEditor && mParent, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIDOMCharacterData>nodeAsText = do_QueryInterface(mNewNode);
  if (nodeAsText)
  {
    nodeAsText->SetData(EmptyString());
  }

  
  ErrorResult rv;
  mParent->InsertBefore(*mNewNode, mRefNode, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP CreateElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("CreateElementTxn: ");
  aString += mTag;
  return NS_OK;
}

NS_IMETHODIMP CreateElementTxn::GetNewNode(nsINode **aNewNode)
{
  NS_ENSURE_TRUE(aNewNode, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(mNewNode, NS_ERROR_NOT_INITIALIZED);
  *aNewNode = mNewNode;
  NS_ADDREF(*aNewNode);
  return NS_OK;
}
