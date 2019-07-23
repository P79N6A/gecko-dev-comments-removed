




































#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "DeleteElementTxn.h"
#include "nsSelectionState.h"
#ifdef NS_DEBUG
#include "nsIDOMElement.h"
#endif

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#endif


DeleteElementTxn::DeleteElementTxn()
: EditTxn()
,mElement()
,mParent()
,mRefNode()
,mRangeUpdater(nsnull)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(DeleteElementTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DeleteElementTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRefNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DeleteElementTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRefNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DeleteElementTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP DeleteElementTxn::Init(nsIEditor *aEditor,
                                     nsIDOMNode *aElement,
                                     nsRangeUpdater *aRangeUpdater)
{
  if (!aEditor || !aElement) return NS_ERROR_NULL_POINTER;
  mEditor = aEditor;
  mElement = do_QueryInterface(aElement);
  nsresult result = mElement->GetParentNode(getter_AddRefs(mParent));
  if (NS_FAILED(result)) { return result; }

  
  if (mParent && !mEditor->IsModifiableNode(mParent)) {
    return NS_ERROR_FAILURE;
  }

  mRangeUpdater = aRangeUpdater;
  return NS_OK;
}


NS_IMETHODIMP DeleteElementTxn::DoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy)
  {
    printf("%p Do Delete Element element = %p\n",
           static_cast<void*>(this),
           static_cast<void*>(mElement.get()));
  }
#endif

  if (!mElement) return NS_ERROR_NOT_INITIALIZED;

  if (!mParent) { return NS_OK; }  

#ifdef NS_DEBUG
  
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(mElement);
  nsAutoString elementTag(NS_LITERAL_STRING("text node"));
  if (element)
    element->GetTagName(elementTag);
  nsCOMPtr<nsIDOMElement> parentElement = do_QueryInterface(mParent);
  nsAutoString parentElementTag(NS_LITERAL_STRING("text node"));
  if (parentElement)
    parentElement->GetTagName(parentElementTag);
  char *c, *p;
  c = ToNewCString(elementTag);
  p = ToNewCString(parentElementTag);
  if (c&&p)
  {
    if (gNoisy)
      printf("  DeleteElementTxn:  deleting child %s from parent %s\n", c, p); 

    NS_Free(c);
    NS_Free(p);
  }
  
#endif

  
  mElement->GetNextSibling(getter_AddRefs(mRefNode));  

  
  
  if (mRangeUpdater) 
    mRangeUpdater->SelAdjDeleteNode(mElement);

  nsCOMPtr<nsIDOMNode> resultNode;
  return mParent->RemoveChild(mElement, getter_AddRefs(resultNode));
}

NS_IMETHODIMP DeleteElementTxn::UndoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy)
  {
    printf("%p Undo Delete Element element = %p, parent = %p\n",
           static_cast<void*>(this),
           static_cast<void*>(mElement.get()),
           static_cast<void*>(mParent.get()));
  }
#endif

  if (!mParent) { return NS_OK; } 
  if (!mElement) { return NS_ERROR_NULL_POINTER; }

#ifdef NS_DEBUG
  
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(mElement);
  nsAutoString elementTag(NS_LITERAL_STRING("text node"));
  if (element)
    element->GetTagName(elementTag);
  nsCOMPtr<nsIDOMElement> parentElement = do_QueryInterface(mParent);
  nsAutoString parentElementTag(NS_LITERAL_STRING("text node"));
  if (parentElement)
    parentElement->GetTagName(parentElementTag);
  char *c, *p;
  c = ToNewCString(elementTag);
  p = ToNewCString(parentElementTag);
  if (c&&p)
  {
    if (gNoisy)
      printf("  DeleteElementTxn:  inserting child %s back into parent %s\n", c, p); 

    NS_Free(c);
    NS_Free(p);
  }
  
#endif

  nsCOMPtr<nsIDOMNode> resultNode;
  return mParent->InsertBefore(mElement, mRefNode, getter_AddRefs(resultNode));
}

NS_IMETHODIMP DeleteElementTxn::RedoTransaction(void)
{
#ifdef NS_DEBUG
  if (gNoisy)
  {
    printf("%p Redo Delete Element element = %p, parent = %p\n",
           static_cast<void*>(this),
           static_cast<void*>(mElement.get()),
           static_cast<void*>(mParent.get()));
  }
#endif

  if (!mParent) { return NS_OK; } 
  if (!mElement) { return NS_ERROR_NULL_POINTER; }

  if (mRangeUpdater) 
    mRangeUpdater->SelAdjDeleteNode(mElement);

  nsCOMPtr<nsIDOMNode> resultNode;
  return mParent->RemoveChild(mElement, getter_AddRefs(resultNode));
}

NS_IMETHODIMP DeleteElementTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("DeleteElementTxn");
  return NS_OK;
}
