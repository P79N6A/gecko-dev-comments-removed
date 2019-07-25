



#ifndef nsPerformance_h___
#define nsPerformance_h___

#include "nsIDOMPerformance.h"
#include "nsIDOMPerformanceTiming.h"
#include "nsIDOMPerformanceNavigation.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#include "nsWrapperCache.h"
#include "nsDOMNavigationTiming.h"

class nsIURI;
class nsITimedChannel;
class nsPerformance;
struct JSObject;
struct JSContext;


class nsPerformanceTiming MOZ_FINAL : public nsIDOMPerformanceTiming,
                                      public nsWrapperCache
{
public:
  nsPerformanceTiming(nsPerformance* aPerformance,
                      nsITimedChannel* aChannel);
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCETIMING
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsPerformanceTiming)

  nsDOMNavigationTiming* GetDOMTiming() const;

  nsPerformance* GetParentObject() const
  {
    return mPerformance;
  }

  JSObject* WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap);

  
  DOMTimeMilliSec GetNavigationStart() const {
    return GetDOMTiming()->GetNavigationStart();
  }
  DOMTimeMilliSec GetUnloadEventStart() {
    return GetDOMTiming()->GetUnloadEventStart();
  }
  DOMTimeMilliSec GetUnloadEventEnd() {
    return GetDOMTiming()->GetUnloadEventEnd();
  }
  DOMTimeMilliSec GetRedirectStart() {
    return GetDOMTiming()->GetRedirectStart();
  }
  DOMTimeMilliSec GetRedirectEnd() {
    return GetDOMTiming()->GetRedirectEnd();
  }
  DOMTimeMilliSec GetFetchStart() const {
    return GetDOMTiming()->GetFetchStart();
  }
  DOMTimeMilliSec GetDomainLookupStart() const;
  DOMTimeMilliSec GetDomainLookupEnd() const;
  DOMTimeMilliSec GetConnectStart() const;
  DOMTimeMilliSec GetConnectEnd() const;
  DOMTimeMilliSec GetRequestStart() const;
  DOMTimeMilliSec GetResponseStart() const;
  DOMTimeMilliSec GetResponseEnd() const;
  DOMTimeMilliSec GetDomLoading() const {
    return GetDOMTiming()->GetDomLoading();
  }
  DOMTimeMilliSec GetDomInteractive() const {
    return GetDOMTiming()->GetDomInteractive();
  }
  DOMTimeMilliSec GetDomContentLoadedEventStart() const {
    return GetDOMTiming()->GetDomContentLoadedEventStart();
  }
  DOMTimeMilliSec GetDomContentLoadedEventEnd() const {
    return GetDOMTiming()->GetDomContentLoadedEventEnd();
  }
  DOMTimeMilliSec GetDomComplete() const {
    return GetDOMTiming()->GetDomComplete();
  }
  DOMTimeMilliSec GetLoadEventStart() const {
    return GetDOMTiming()->GetLoadEventStart();
  }
  DOMTimeMilliSec GetLoadEventEnd() const {
    return GetDOMTiming()->GetLoadEventEnd();
  }

private:
  ~nsPerformanceTiming();
  nsRefPtr<nsPerformance> mPerformance;
  nsCOMPtr<nsITimedChannel> mChannel;
};


class nsPerformanceNavigation MOZ_FINAL : public nsIDOMPerformanceNavigation,
                                          public nsWrapperCache
{
public:
  explicit nsPerformanceNavigation(nsPerformance* aPerformance);
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCENAVIGATION
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsPerformanceNavigation)

  nsDOMNavigationTiming* GetDOMTiming() const;

  nsPerformance* GetParentObject() const
  {
    return mPerformance;
  }

  JSObject* WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap);

  
  PRUint16 GetType() const {
    return GetDOMTiming()->GetType();
  }
  PRUint16 GetRedirectCount() const {
    return GetDOMTiming()->GetRedirectCount();
  }

private:
  ~nsPerformanceNavigation();
  nsRefPtr<nsPerformance> mPerformance;
};


class nsPerformance MOZ_FINAL : public nsIDOMPerformance,
                                public nsWrapperCache
{
public:
  nsPerformance(nsIDOMWindow* aWindow,
                nsDOMNavigationTiming* aDOMTiming,
                nsITimedChannel* aChannel);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMPERFORMANCE
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsPerformance)

  nsDOMNavigationTiming* GetDOMTiming() const
  {
    return mDOMTiming;
  }

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow.get();
  }

  JSObject* WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap);

  
  DOMHighResTimeStamp Now();
  nsPerformanceTiming* GetTiming();
  nsPerformanceNavigation* GetNavigation();

private:
  ~nsPerformance();

  nsCOMPtr<nsIDOMWindow> mWindow;
  nsRefPtr<nsDOMNavigationTiming> mDOMTiming;
  nsCOMPtr<nsITimedChannel> mChannel;
  nsRefPtr<nsPerformanceTiming> mTiming;
  nsRefPtr<nsPerformanceNavigation> mNavigation;
};

inline nsDOMNavigationTiming*
nsPerformanceNavigation::GetDOMTiming() const
{
  return mPerformance->GetDOMTiming();
}

inline nsDOMNavigationTiming*
nsPerformanceTiming::GetDOMTiming() const
{
  return mPerformance->GetDOMTiming();
}

#endif 

