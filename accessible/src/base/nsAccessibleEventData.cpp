





































#include "nsAccessibleEventData.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessNode.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS1(nsAccessibleEventData, nsIAccessibleEvent)

nsAccessibleEventData::nsAccessibleEventData(PRUint32 aEventType, nsIAccessible *aAccessible, 
                                             nsIAccessibleDocument *aDocAccessible, 
                                             void *aEventData):
  mEventType(aEventType), mAccessible(aAccessible), mDocAccessible(aDocAccessible), 
  mEventData(aEventData)
{
}

nsAccessibleEventData::nsAccessibleEventData(PRUint32 aEventType, nsIDOMNode *aDOMNode,
                                             nsIAccessibleDocument *aDocAccessible, 
                                             void *aEventData):
  mEventType(aEventType), mDOMNode(aDOMNode), mDocAccessible(aDocAccessible),
  mEventData(aEventData)
{
}

NS_IMETHODIMP nsAccessibleEventData::GetAccessible(nsIAccessible **aAccessible) 
{
  *aAccessible = nsnull;
  if (!mAccessible) {
    NS_ENSURE_TRUE(mDOMNode, NS_ERROR_FAILURE);
    nsCOMPtr<nsIAccessibilityService> accService = 
      do_GetService("@mozilla.org/accessibilityService;1");
    NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);
    accService->GetAccessibleFor(mDOMNode, getter_AddRefs(mAccessible));
    if (!mAccessible) {
      return NS_OK;
    }
  }
  NS_ADDREF(*aAccessible = mAccessible);
  return NS_OK;
}

NS_IMETHODIMP nsAccessibleEventData::GetDOMNode(nsIDOMNode **aDOMNode)
{
  if (!mDOMNode) {
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(mAccessible));
    NS_ENSURE_TRUE(accessNode, NS_ERROR_FAILURE);
    accessNode->GetDOMNode(getter_AddRefs(mDOMNode));
  }
  NS_ADDREF(*aDOMNode = mDOMNode);
  return NS_OK;
}
