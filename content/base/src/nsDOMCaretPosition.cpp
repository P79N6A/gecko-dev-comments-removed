




































#include "nsDOMCaretPosition.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMClassInfo.h"

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMCaretPosition)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCaretPosition)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMCaretPosition)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CaretPosition)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_1(nsDOMCaretPosition, mNode)
 
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMCaretPosition)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMCaretPosition)

DOMCI_DATA(CaretPosition, nsDOMCaretPosition)


nsDOMCaretPosition::nsDOMCaretPosition(nsIDOMNode* aNode, PRUint32 aOffset)
  : mNode(aNode), mOffset(aOffset)
{
}

nsDOMCaretPosition::~nsDOMCaretPosition()
{
}

NS_IMETHODIMP nsDOMCaretPosition::GetOffsetNode(nsIDOMNode** aOffsetNode)
{
  nsCOMPtr<nsIDOMNode> node = mNode;
  node.forget(aOffsetNode);
  return NS_OK;
}

NS_IMETHODIMP nsDOMCaretPosition::GetOffset(PRUint32* aOffset)
{
  *aOffset = mOffset;
  return NS_OK;
}

