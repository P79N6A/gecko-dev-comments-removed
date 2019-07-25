






































#ifndef nsDOMNavigationTiming_h___
#define nsDOMNavigationTiming_h___

#include "nsIDOMPerformanceTiming.h"
#include "nsIDOMPerformanceNavigation.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "TimeStamp.h"

class nsDOMNavigationTimingClock;
class nsIURI;
class nsIDocument;

class nsDOMNavigationTiming : public nsIDOMPerformanceTiming,
                              public nsIDOMPerformanceNavigation
{
public:
  nsDOMNavigationTiming();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCETIMING
  NS_DECL_NSIDOMPERFORMANCENAVIGATION

  void NotifyNavigationStart();
  void NotifyFetchStart(nsIURI* aURI, nsDOMPerformanceNavigationType aNavigationType);
  void NotifyRedirect(nsIURI* aOldURI, nsIURI* aNewURI);
  void NotifyBeforeUnload();
  void NotifyUnloadAccepted(nsIURI* aOldURI);
  void NotifyUnloadEventStart();
  void NotifyUnloadEventEnd();
  void NotifyLoadEventStart();
  void NotifyLoadEventEnd();

  
  void SetDOMLoadingTimeStamp(nsIURI* aURI, mozilla::TimeStamp aValue);
  void NotifyDOMLoading(nsIURI* aURI);
  void NotifyDOMInteractive(nsIURI* aURI);
  void NotifyDOMComplete(nsIURI* aURI);
  void NotifyDOMContentLoadedStart(nsIURI* aURI);
  void NotifyDOMContentLoadedEnd(nsIURI* aURI);

private:
  nsDOMNavigationTiming(const nsDOMNavigationTiming &){};
  ~nsDOMNavigationTiming();

  void Clear();
  PRBool ReportRedirects();

  nsCOMPtr<nsIURI> mUnloadedURI;
  nsCOMPtr<nsIURI> mLoadedURI;
  nsCOMArray<nsIURI> mRedirects;

  typedef enum { NOT_CHECKED,
                 CHECK_PASSED,
                 NO_REDIRECTS,
                 CHECK_FAILED} RedirectCheckState;
  RedirectCheckState mRedirectCheck;

  nsDOMPerformanceNavigationType mNavigationType;
  DOMTimeMilliSec mNavigationStart;
  mozilla::TimeStamp mNavigationStartTimeStamp;
  DOMTimeMilliSec DurationFromStart();

  DOMTimeMilliSec mFetchStart;
  DOMTimeMilliSec mRedirectStart;
  DOMTimeMilliSec mRedirectEnd;
  DOMTimeMilliSec mBeforeUnloadStart;
  DOMTimeMilliSec mUnloadStart;
  DOMTimeMilliSec mUnloadEnd;
  DOMTimeMilliSec mNavigationEnd;
  DOMTimeMilliSec mLoadEventStart;
  DOMTimeMilliSec mLoadEventEnd;

  DOMTimeMilliSec mDOMLoading;
  DOMTimeMilliSec mDOMInteractive;
  DOMTimeMilliSec mDOMContentLoadedEventStart;
  DOMTimeMilliSec mDOMContentLoadedEventEnd;
  DOMTimeMilliSec mDOMComplete;
};

#endif 
