






































#include "nsPerformance.h"
#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsIDocShell.h"
#include "nsDOMClassInfo.h"
#include "nsDOMNavigationTiming.h"

DOMCI_DATA(PerformanceTiming, nsPerformanceTiming)

NS_IMPL_ADDREF(nsPerformanceTiming)
NS_IMPL_RELEASE(nsPerformanceTiming)


NS_INTERFACE_MAP_BEGIN(nsPerformanceTiming)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMPerformanceTiming)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPerformanceTiming)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(PerformanceTiming)
NS_INTERFACE_MAP_END

nsPerformanceTiming::nsPerformanceTiming(nsDOMNavigationTiming* aData)
{
  NS_ASSERTION(aData, "Timing data should be provided");
  mData = aData;
}

nsPerformanceTiming::~nsPerformanceTiming()
{
}

NS_IMETHODIMP
nsPerformanceTiming::GetNavigationStart(DOMTimeMilliSec* aTime)
{
  return mData->GetNavigationStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetUnloadEventStart(DOMTimeMilliSec* aTime)
{
  return mData->GetUnloadEventStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetUnloadEventEnd(DOMTimeMilliSec* aTime)
{
  return mData->GetUnloadEventEnd(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetRedirectStart(DOMTimeMilliSec* aTime)
{
  return mData->GetRedirectStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetRedirectEnd(DOMTimeMilliSec* aTime)
{
  return mData->GetRedirectEnd(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetFetchStart(DOMTimeMilliSec* aTime)
{
  return mData->GetFetchStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetDomainLookupStart(DOMTimeMilliSec* aTime)
{
  return mData->GetDomainLookupStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetDomainLookupEnd(DOMTimeMilliSec* aTime)
{
  return mData->GetDomainLookupEnd(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetConnectStart(DOMTimeMilliSec* aTime)
{
  return mData->GetConnectStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetConnectEnd(DOMTimeMilliSec* aTime)
{
  return mData->GetConnectEnd(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetHandshakeStart(DOMTimeMilliSec* aTime)
{
  return mData->GetHandshakeStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetRequestStart(DOMTimeMilliSec* aTime)
{
  return mData->GetRequestStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetResponseStart(DOMTimeMilliSec* aTime)
{
  return mData->GetResponseStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetResponseEnd(DOMTimeMilliSec* aTime)
{
  return mData->GetResponseEnd(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetDomLoading(DOMTimeMilliSec* aTime)
{
  return mData->GetDomLoading(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetDomInteractive(DOMTimeMilliSec* aTime)
{
  return mData->GetDomInteractive(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetDomContentLoadedEventStart(DOMTimeMilliSec* aTime)
{
  return mData->GetDomContentLoadedEventStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetDomContentLoadedEventEnd(DOMTimeMilliSec* aTime)
{
  return mData->GetDomContentLoadedEventEnd(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetDomComplete(DOMTimeMilliSec* aTime)
{
  return mData->GetDomComplete(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetLoadEventStart(DOMTimeMilliSec* aTime)
{
  return mData->GetLoadEventStart(aTime);
}

NS_IMETHODIMP
nsPerformanceTiming::GetLoadEventEnd(DOMTimeMilliSec* aTime)
{
  return mData->GetLoadEventEnd(aTime);
}



DOMCI_DATA(PerformanceNavigation, nsPerformanceNavigation)

NS_IMPL_ADDREF(nsPerformanceNavigation)
NS_IMPL_RELEASE(nsPerformanceNavigation)


NS_INTERFACE_MAP_BEGIN(nsPerformanceNavigation)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMPerformanceNavigation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPerformanceNavigation)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(PerformanceNavigation)
NS_INTERFACE_MAP_END

nsPerformanceNavigation::nsPerformanceNavigation(nsDOMNavigationTiming* aData)
{
  NS_ASSERTION(aData, "Timing data should be provided");
  mData = aData;
}

nsPerformanceNavigation::~nsPerformanceNavigation()
{
}

NS_IMETHODIMP
nsPerformanceNavigation::GetType(
    nsDOMPerformanceNavigationType* aNavigationType)
{
  return mData->GetType(aNavigationType);
}

NS_IMETHODIMP
nsPerformanceNavigation::GetRedirectCount(PRUint16* aRedirectCount)
{
  return mData->GetRedirectCount(aRedirectCount);
}


DOMCI_DATA(Performance, nsPerformance)

NS_IMPL_ADDREF(nsPerformance)
NS_IMPL_RELEASE(nsPerformance)

nsPerformance::nsPerformance(nsDOMNavigationTiming* aTiming)
{
  NS_ASSERTION(aTiming, "Timing data should be provided");
  mData = aTiming;
}

nsPerformance::~nsPerformance()
{
}


NS_INTERFACE_MAP_BEGIN(nsPerformance)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMPerformance)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPerformance)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Performance)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsPerformance::GetTiming(nsIDOMPerformanceTiming** aTiming)
{
  if (!mTiming) {
    mTiming = new nsPerformanceTiming(mData);
  }
  NS_IF_ADDREF(*aTiming = mTiming);
  return NS_OK;
}

NS_IMETHODIMP
nsPerformance::GetNavigation(nsIDOMPerformanceNavigation** aNavigation)
{
  if (!mNavigation) {
    mNavigation = new nsPerformanceNavigation(mData);
  }
  NS_IF_ADDREF(*aNavigation = mNavigation);
  return NS_OK;
}
