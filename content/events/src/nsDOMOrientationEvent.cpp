



































#include "nsDOMOrientationEvent.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF_INHERITED(nsDOMOrientationEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMOrientationEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMOrientationEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMOrientationEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(OrientationEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP nsDOMOrientationEvent::InitOrientationEvent(const nsAString & eventTypeArg,
                                                          PRBool canBubbleArg,
                                                          PRBool cancelableArg,
                                                          double x,
                                                          double y,
                                                          double z)
{
  nsresult rv = nsDOMEvent::InitEvent(eventTypeArg, canBubbleArg, cancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mX = x;
  mY = y;
  mZ = z;

  return NS_OK;
}


NS_IMETHODIMP nsDOMOrientationEvent::GetX(double *aX)
{
  NS_ENSURE_ARG_POINTER(aX);

  *aX = mX;
  return NS_OK;
}

NS_IMETHODIMP nsDOMOrientationEvent::GetY(double *aY)
{
  NS_ENSURE_ARG_POINTER(aY);

  *aY = mY;
  return NS_OK;
}

NS_IMETHODIMP nsDOMOrientationEvent::GetZ(double *aZ)
{
  NS_ENSURE_ARG_POINTER(aZ);

  *aZ = mZ;
  return NS_OK;
}


nsresult NS_NewDOMOrientationEvent(nsIDOMEvent** aInstancePtrResult,
                                   nsPresContext* aPresContext,
                                   nsEvent *aEvent) 
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsDOMOrientationEvent* it = new nsDOMOrientationEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
