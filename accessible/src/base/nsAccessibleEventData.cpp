





































#include "nsAccessibleEventData.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessNode.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS1(nsAccEvent, nsIAccessibleEvent)

nsAccEvent::nsAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                       void *aEventData):
  mEventType(aEventType), mAccessible(aAccessible), mEventData(aEventData)
{
}

nsAccEvent::nsAccEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode,
                       void *aEventData):
  mEventType(aEventType), mDOMNode(aDOMNode), mEventData(aEventData)
{
}

NS_IMETHODIMP
nsAccEvent::GetEventType(PRUint32 *aEventType)
{
  *aEventType = mEventType;
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetAccessible(nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
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
  NS_IF_ADDREF(*aAccessible = mAccessible);
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetDOMNode(nsIDOMNode **aDOMNode)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  *aDOMNode = nsnull;

  if (!mDOMNode) {
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(mAccessible));
    NS_ENSURE_TRUE(accessNode, NS_ERROR_FAILURE);
    accessNode->GetDOMNode(getter_AddRefs(mDOMNode));
  }

  NS_IF_ADDREF(*aDOMNode = mDOMNode);
  return NS_OK;
}

NS_IMETHODIMP
nsAccEvent::GetAccessibleDocument(nsIAccessibleDocument **aDocAccessible)
{
  NS_ENSURE_ARG_POINTER(aDocAccessible);
  *aDocAccessible = nsnull;

  if (!mDocAccessible) {
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(mAccessible));
    NS_ENSURE_TRUE(accessNode, NS_ERROR_FAILURE);
    accessNode->GetAccessibleDocument(getter_AddRefs(mDocAccessible));
  }

  NS_IF_ADDREF(*aDocAccessible = mDocAccessible);
  return NS_OK;
}

