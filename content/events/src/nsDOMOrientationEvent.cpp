



































#include "nsDOMOrientationEvent.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF_INHERITED(nsDOMOrientationEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMOrientationEvent, nsDOMEvent)

DOMCI_DATA(DeviceOrientationEvent, nsDOMOrientationEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMOrientationEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDeviceOrientationEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DeviceOrientationEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP nsDOMOrientationEvent::InitDeviceOrientationEvent(const nsAString & aEventTypeArg,
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

NS_IMETHODIMP nsDOMOrientationEvent::GetAlpha(double *aAlpha)
{
  NS_ENSURE_ARG_POINTER(aAlpha);

  *aAlpha = mAlpha;
  return NS_OK;
}

NS_IMETHODIMP nsDOMOrientationEvent::GetBeta(double *aBeta)
{
  NS_ENSURE_ARG_POINTER(aBeta);

  *aBeta = mBeta;
  return NS_OK;
}

NS_IMETHODIMP nsDOMOrientationEvent::GetGamma(double *aGamma)
{
  NS_ENSURE_ARG_POINTER(aGamma);

  *aGamma = mGamma;
  return NS_OK;
}

NS_IMETHODIMP nsDOMOrientationEvent::GetAbsolute(PRBool *aAbsolute)
{
  NS_ENSURE_ARG_POINTER(aAbsolute);

  *aAbsolute = mAbsolute;
  return NS_OK;
}

NS_IMETHODIMP nsDOMOrientationEvent::GetCompassCalibrated(PRBool *aCompassCalibrated)
{
  NS_ENSURE_ARG_POINTER(aCompassCalibrated);

  *aCompassCalibrated = PR_TRUE;
  return NS_OK;
}

nsresult NS_NewDOMDeviceOrientationEvent(nsIDOMEvent** aInstancePtrResult,
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
