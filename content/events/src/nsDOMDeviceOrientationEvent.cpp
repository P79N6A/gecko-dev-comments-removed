



































#include "nsDOMDeviceOrientationEvent.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF_INHERITED(nsDOMDeviceOrientationEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMDeviceOrientationEvent, nsDOMEvent)

DOMCI_DATA(DeviceOrientationEvent, nsDOMDeviceOrientationEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMDeviceOrientationEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDeviceOrientationEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DeviceOrientationEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP nsDOMDeviceOrientationEvent::InitDeviceOrientationEvent(const nsAString & aEventTypeArg,
                                                                      PRBool aCanBubbleArg,
                                                                      PRBool aCancelableArg,
                                                                      double aAlpha,
                                                                      double aBeta,
                                                                      double aGamma,
                                                                      PRBool aAbsolute)
{
  nsresult rv = nsDOMEvent::InitEvent(aEventTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mAlpha = aAlpha;
  mBeta = aBeta;
  mGamma = aGamma;
  mAbsolute = aAbsolute;

  return NS_OK;
}

NS_IMETHODIMP nsDOMDeviceOrientationEvent::GetAlpha(double *aAlpha)
{
  NS_ENSURE_ARG_POINTER(aAlpha);

  *aAlpha = mAlpha;
  return NS_OK;
}

NS_IMETHODIMP nsDOMDeviceOrientationEvent::GetBeta(double *aBeta)
{
  NS_ENSURE_ARG_POINTER(aBeta);

  *aBeta = mBeta;
  return NS_OK;
}

NS_IMETHODIMP nsDOMDeviceOrientationEvent::GetGamma(double *aGamma)
{
  NS_ENSURE_ARG_POINTER(aGamma);

  *aGamma = mGamma;
  return NS_OK;
}

NS_IMETHODIMP nsDOMDeviceOrientationEvent::GetAbsolute(PRBool *aAbsolute)
{
  NS_ENSURE_ARG_POINTER(aAbsolute);

  *aAbsolute = mAbsolute;
  return NS_OK;
}

nsresult NS_NewDOMDeviceOrientationEvent(nsIDOMEvent** aInstancePtrResult,
                                         nsPresContext* aPresContext,
                                         nsEvent *aEvent) 
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsDOMDeviceOrientationEvent* it = new nsDOMDeviceOrientationEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
