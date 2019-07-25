



#ifndef nsPerformance_h___
#define nsPerformance_h___

#include "nsIDOMPerformance.h"
#include "nsIDOMPerformanceTiming.h"
#include "nsIDOMPerformanceNavigation.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"

class nsIDocument;
class nsIURI;
class nsDOMNavigationTiming;
class nsITimedChannel;


class nsPerformanceTiming MOZ_FINAL : public nsIDOMPerformanceTiming
{
public:
  nsPerformanceTiming(nsDOMNavigationTiming* aDOMTiming, nsITimedChannel* aChannel);
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCETIMING
private:
  ~nsPerformanceTiming();
  nsRefPtr<nsDOMNavigationTiming> mDOMTiming;
  nsCOMPtr<nsITimedChannel> mChannel;
};


class nsPerformanceNavigation MOZ_FINAL : public nsIDOMPerformanceNavigation
{
public:
  nsPerformanceNavigation(nsDOMNavigationTiming* data);
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCENAVIGATION
private:
  ~nsPerformanceNavigation();
  nsRefPtr<nsDOMNavigationTiming> mData;
};


class nsPerformance MOZ_FINAL : public nsIDOMPerformance
{
public:
  nsPerformance(nsDOMNavigationTiming* aDOMTiming, nsITimedChannel* aChannel);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCE

private:
  ~nsPerformance();

  nsRefPtr<nsDOMNavigationTiming> mDOMTiming;
  nsCOMPtr<nsITimedChannel> mChannel;
  nsCOMPtr<nsIDOMPerformanceTiming> mTiming;
  nsCOMPtr<nsIDOMPerformanceNavigation> mNavigation;
};

#endif 

