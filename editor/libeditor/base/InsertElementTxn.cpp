




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

#ifdef DEBUG
static bool gNoisy = false;
#endif


InsertElementTxn::InsertElementTxn()
  : EditTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(InsertElementTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(InsertElementTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mParent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(InsertElementTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mParent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(InsertElementTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(InsertElementTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(InsertElementTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP InsertElementTxn::Init(nsINode *aNode,
                                     nsINode *aParent,
                                     int32_t     aOffset,
                                     nsEditor  *aEditor)
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
#ifdef DEBUG
  if (gNoisy) 
  { 
    nsCOMPtr<nsIContent>nodeAsContent = do_QueryInterface(mNode);
    nsCOMPtr<nsIContent>parentAsContent = do_QueryInterface(mParent);
    nsString namestr;
    mNode->GetNodeName(namestr);
    char* nodename = ToNewCString(namestr);
    printf("%p Do Insert Element of %p <%s> into parent %p at offset %d\n", 
           static_cast<void*>(this),
           static_cast<void*>(nodeAsContent.get()),
           nodename,
           static_cast<void*>(parentAsContent.get()),
           mOffset); 
    nsMemory::Free(nodename);
  }
#endif

  NS_ENSURE_TRUE(mNode && mParent, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsINode> parent = do_QueryInterface(mParent);
  NS_ENSURE_STATE(parent);

  uint32_t count = parent->GetChildCount();
  if (mOffset > int32_t(count) || mOffset == -1) {
    
    mOffset = count;
  }

  
  nsCOMPtr<nsIContent> refContent = parent->GetChildAt(mOffset);

  mEditor->MarkNodeDirty(mNode);

  ErrorResult rv;
  mParent->InsertBefore(*mNode, refContent, rv);
  NS_ENSURE_SUCCESS(rv.ErrorCode(), rv.ErrorCode());

  
  bool bAdjustSelection;
  mEditor->ShouldTxnSetSelection(&bAdjustSelection);
  if (bAdjustSelection)
  {
    nsCOMPtr<nsISelection> selection;
    rv = mEditor->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(rv.ErrorCode(), rv.ErrorCode());
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    
    selection->Collapse(mParent->AsDOMNode(), mOffset+1);
  }
  else
  {
    
  }
  return NS_OK;
}

NS_IMETHODIMP InsertElementTxn::UndoTransaction(void)
{
#ifdef DEBUG
  if (gNoisy)
  {
    printf("%p Undo Insert Element of %p into parent %p at offset %d\n",
           static_cast<void*>(this),
           static_cast<void*>(mNode.get()),
           static_cast<void*>(mParent.get()),
           mOffset);
  }
#endif

  NS_ENSURE_TRUE(mNode && mParent, NS_ERROR_NOT_INITIALIZED);

  ErrorResult rv;
  mParent->RemoveChild(*mNode, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP InsertElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("InsertElementTxn");
  return NS_OK;
}
