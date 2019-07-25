





































#ifndef nsPerformance_h___
#define nsPerformance_h___

#include "nsIDOMPerformance.h"
#include "nsIDOMPerformanceTiming.h"
#include "nsIDOMPerformanceNavigation.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

class nsIDocument;
class nsIURI;
class nsDOMNavigationTiming;


class nsPerformanceTiming : public nsIDOMPerformanceTiming
{
public:
  nsPerformanceTiming(nsDOMNavigationTiming* data);
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCETIMING
private:
  ~nsPerformanceTiming();
  nsRefPtr<nsDOMNavigationTiming> mData;
};


class nsPerformanceNavigation : public nsIDOMPerformanceNavigation
{
public:
  nsPerformanceNavigation(nsDOMNavigationTiming* data);
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCENAVIGATION
private:
  ~nsPerformanceNavigation();
  nsRefPtr<nsDOMNavigationTiming> mData;
};


class nsPerformance : public nsIDOMPerformance
{
public:
  nsPerformance(nsDOMNavigationTiming* timing);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCE

private:
  ~nsPerformance();

  nsRefPtr<nsDOMNavigationTiming> mData;
  nsCOMPtr<nsIDOMPerformanceTiming> mTiming;
  nsCOMPtr<nsIDOMPerformanceNavigation> mNavigation;
};

#endif 

