




































#include "nsCOMPtr.h"
#include "nsDOMMutationEvent.h"
#include "nsMutationEvent.h"
#include "nsContentUtils.h"

class nsPresContext;

nsDOMMutationEvent::nsDOMMutationEvent(nsPresContext* aPresContext,
                                       nsMutationEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent :
               new nsMutationEvent(PR_FALSE, 0))
{
  mEventIsInternal = (aEvent == nsnull);
}

nsDOMMutationEvent::~nsDOMMutationEvent()
{
  if (mEventIsInternal) {
    nsMutationEvent* mutation = NS_STATIC_CAST(nsMutationEvent*, mEvent);
    delete mutation;
    mEvent = nsnull;
  }
}

NS_INTERFACE_MAP_BEGIN(nsDOMMutationEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMutationEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(MutationEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMMutationEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMMutationEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMMutationEvent::GetRelatedNode(nsIDOMNode** aRelatedNode)
{
  *aRelatedNode = nsnull;
  nsMutationEvent* mutation = NS_STATIC_CAST(nsMutationEvent*, mEvent);
  *aRelatedNode = mutation->mRelatedNode;
  NS_IF_ADDREF(*aRelatedNode);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMutationEvent::GetPrevValue(nsAString& aPrevValue)
{
  nsMutationEvent* mutation = NS_STATIC_CAST(nsMutationEvent*, mEvent);
  if (mutation->mPrevAttrValue)
    mutation->mPrevAttrValue->ToString(aPrevValue);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMutationEvent::GetNewValue(nsAString& aNewValue)
{
  nsMutationEvent* mutation = NS_STATIC_CAST(nsMutationEvent*, mEvent);
  if (mutation->mNewAttrValue)
      mutation->mNewAttrValue->ToString(aNewValue);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMutationEvent::GetAttrName(nsAString& aAttrName)
{
  nsMutationEvent* mutation = NS_STATIC_CAST(nsMutationEvent*, mEvent);
  if (mutation->mAttrName)
      mutation->mAttrName->ToString(aAttrName);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMutationEvent::GetAttrChange(PRUint16* aAttrChange)
{
  *aAttrChange = 0;
  nsMutationEvent* mutation = NS_STATIC_CAST(nsMutationEvent*, mEvent);
  if (mutation->mAttrChange)
      *aAttrChange = mutation->mAttrChange;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMutationEvent::InitMutationEvent(const nsAString& aTypeArg, PRBool aCanBubbleArg, PRBool aCancelableArg, nsIDOMNode* aRelatedNodeArg, const nsAString& aPrevValueArg, const nsAString& aNewValueArg, const nsAString& aAttrNameArg, PRUint16 aAttrChangeArg)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsMutationEvent* mutation = NS_STATIC_CAST(nsMutationEvent*, mEvent);
  mutation->mRelatedNode = aRelatedNodeArg;
  if (!aPrevValueArg.IsEmpty())
    mutation->mPrevAttrValue = do_GetAtom(aPrevValueArg);
  if (!aNewValueArg.IsEmpty())
    mutation->mNewAttrValue = do_GetAtom(aNewValueArg);
  if (!aAttrNameArg.IsEmpty()) {
    mutation->mAttrName = do_GetAtom(aAttrNameArg);
  }
  mutation->mAttrChange = aAttrChangeArg;
    
  return NS_OK;
}

nsresult NS_NewDOMMutationEvent(nsIDOMEvent** aInstancePtrResult,
                                nsPresContext* aPresContext,
                                nsMutationEvent *aEvent) 
{
  nsDOMMutationEvent* it = new nsDOMMutationEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
