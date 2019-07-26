



#ifndef nsPerformance_h___
#define nsPerformance_h___

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#include "nsWrapperCache.h"
#include "nsDOMNavigationTiming.h"

class nsIURI;
class nsITimedChannel;
class nsIDOMWindow;
class nsPerformance;
class JSObject;
struct JSContext;


class nsPerformanceTiming MOZ_FINAL : public nsWrapperCache
{
public:
  nsPerformanceTiming(nsPerformance* aPerformance,
                      nsITimedChannel* aChannel);
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsPerformanceTiming)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsPerformanceTiming)

  nsDOMNavigationTiming* GetDOMTiming() const;

  nsPerformance* GetParentObject() const
  {
    return mPerformance;
  }

  virtual JSObject* WrapObject(JSContext *cx,
                               JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

  
  DOMTimeMilliSec NavigationStart() const {
    return GetDOMTiming()->GetNavigationStart();
  }
  DOMTimeMilliSec UnloadEventStart() {
    return GetDOMTiming()->GetUnloadEventStart();
  }
  DOMTimeMilliSec UnloadEventEnd() {
    return GetDOMTiming()->GetUnloadEventEnd();
  }
  DOMTimeMilliSec RedirectStart() {
    return GetDOMTiming()->GetRedirectStart();
  }
  DOMTimeMilliSec RedirectEnd() {
    return GetDOMTiming()->GetRedirectEnd();
  }
  DOMTimeMilliSec FetchStart() const {
    return GetDOMTiming()->GetFetchStart();
  }
  DOMTimeMilliSec DomainLookupStart() const;
  DOMTimeMilliSec DomainLookupEnd() const;
  DOMTimeMilliSec ConnectStart() const;
  DOMTimeMilliSec ConnectEnd() const;
  DOMTimeMilliSec RequestStart() const;
  DOMTimeMilliSec ResponseStart() const;
  DOMTimeMilliSec ResponseEnd() const;
  DOMTimeMilliSec DomLoading() const {
    return GetDOMTiming()->GetDomLoading();
  }
  DOMTimeMilliSec DomInteractive() const {
    return GetDOMTiming()->GetDomInteractive();
  }
  DOMTimeMilliSec DomContentLoadedEventStart() const {
    return GetDOMTiming()->GetDomContentLoadedEventStart();
  }
  DOMTimeMilliSec DomContentLoadedEventEnd() const {
    return GetDOMTiming()->GetDomContentLoadedEventEnd();
  }
  DOMTimeMilliSec DomComplete() const {
    return GetDOMTiming()->GetDomComplete();
  }
  DOMTimeMilliSec LoadEventStart() const {
    return GetDOMTiming()->GetLoadEventStart();
  }
  DOMTimeMilliSec LoadEventEnd() const {
    return GetDOMTiming()->GetLoadEventEnd();
  }

private:
  ~nsPerformanceTiming();
  nsRefPtr<nsPerformance> mPerformance;
  nsCOMPtr<nsITimedChannel> mChannel;
};


class nsPerformanceNavigation MOZ_FINAL : public nsWrapperCache
{
public:
  explicit nsPerformanceNavigation(nsPerformance* aPerformance);
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsPerformanceNavigation)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsPerformanceNavigation)

  nsDOMNavigationTiming* GetDOMTiming() const;

  nsPerformance* GetParentObject() const
  {
    return mPerformance;
  }

  virtual JSObject* WrapObject(JSContext *cx,
                               JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

  
  uint16_t Type() const {
    return GetDOMTiming()->GetType();
  }
  uint16_t RedirectCount() const {
    return GetDOMTiming()->GetRedirectCount();
  }

private:
  ~nsPerformanceNavigation();
  nsRefPtr<nsPerformance> mPerformance;
};


class nsPerformance MOZ_FINAL : public nsISupports,
                                public nsWrapperCache
{
public:
  nsPerformance(nsIDOMWindow* aWindow,
                nsDOMNavigationTiming* aDOMTiming,
                nsITimedChannel* aChannel);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsPerformance)

  nsDOMNavigationTiming* GetDOMTiming() const
  {
    return mDOMTiming;
  }

  nsITimedChannel* GetChannel() const
  {
    return mChannel;
  }

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow.get();
  }

  virtual JSObject* WrapObject(JSContext *cx,
                               JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

  
  DOMHighResTimeStamp Now();
  nsPerformanceTiming* Timing();
  nsPerformanceNavigation* Navigation();

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

