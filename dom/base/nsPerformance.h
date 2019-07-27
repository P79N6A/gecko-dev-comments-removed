



#ifndef nsPerformance_h___
#define nsPerformance_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#include "nsWrapperCache.h"
#include "nsDOMNavigationTiming.h"
#include "nsContentUtils.h"
#include "nsPIDOMWindow.h"
#include "js/TypeDecls.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/DOMEventTargetHelper.h"

class nsITimedChannel;
class nsPerformance;
class nsIHttpChannel;

namespace mozilla {
namespace dom {
  class PerformanceEntry;
}
}


class nsPerformanceTiming MOZ_FINAL : public nsWrapperCache
{
public:
  typedef mozilla::TimeStamp TimeStamp;




















  nsPerformanceTiming(nsPerformance* aPerformance,
                      nsITimedChannel* aChannel,
                      nsIHttpChannel* aHttpChannel,
                      DOMHighResTimeStamp aZeroTime);
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsPerformanceTiming)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsPerformanceTiming)

  nsDOMNavigationTiming* GetDOMTiming() const;

  nsPerformance* GetParentObject() const
  {
    return mPerformance;
  }

  








  inline DOMHighResTimeStamp TimeStampToDOMHighResOrFetchStart(TimeStamp aStamp)
  {
    return (!aStamp.IsNull())
        ? TimeStampToDOMHighRes(aStamp)
        : FetchStartHighRes();
  }

  


























  inline DOMHighResTimeStamp TimeStampToDOMHighRes(TimeStamp aStamp) const
  {
    MOZ_ASSERT(!aStamp.IsNull());
    mozilla::TimeDuration duration =
        aStamp - GetDOMTiming()->GetNavigationStartTimeStamp();
    return duration.ToMilliseconds() + mZeroTime;
  }

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

  
  DOMTimeMilliSec NavigationStart() const {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetNavigationStart();
  }
  DOMTimeMilliSec UnloadEventStart() {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetUnloadEventStart();
  }
  DOMTimeMilliSec UnloadEventEnd() {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetUnloadEventEnd();
  }

  uint16_t GetRedirectCount() const;
  
  
  
  bool CheckAllowedOrigin(nsIHttpChannel* aResourceChannel);
  
  
  bool TimingAllowed() const;

  
  
  
  bool ShouldReportCrossOriginRedirect() const;

  
  DOMHighResTimeStamp FetchStartHighRes();
  DOMHighResTimeStamp RedirectStartHighRes();
  DOMHighResTimeStamp RedirectEndHighRes();
  DOMHighResTimeStamp DomainLookupStartHighRes();
  DOMHighResTimeStamp DomainLookupEndHighRes();
  DOMHighResTimeStamp ConnectStartHighRes();
  DOMHighResTimeStamp ConnectEndHighRes();
  DOMHighResTimeStamp RequestStartHighRes();
  DOMHighResTimeStamp ResponseStartHighRes();
  DOMHighResTimeStamp ResponseEndHighRes();

  
  DOMTimeMilliSec FetchStart();
  DOMTimeMilliSec RedirectStart();
  DOMTimeMilliSec RedirectEnd();
  DOMTimeMilliSec DomainLookupStart();
  DOMTimeMilliSec DomainLookupEnd();
  DOMTimeMilliSec ConnectStart();
  DOMTimeMilliSec ConnectEnd();
  DOMTimeMilliSec RequestStart();
  DOMTimeMilliSec ResponseStart();
  DOMTimeMilliSec ResponseEnd();

  DOMTimeMilliSec DomLoading() {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetDomLoading();
  }
  DOMTimeMilliSec DomInteractive() const {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetDomInteractive();
  }
  DOMTimeMilliSec DomContentLoadedEventStart() const {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetDomContentLoadedEventStart();
  }
  DOMTimeMilliSec DomContentLoadedEventEnd() const {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetDomContentLoadedEventEnd();
  }
  DOMTimeMilliSec DomComplete() const {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetDomComplete();
  }
  DOMTimeMilliSec LoadEventStart() const {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetLoadEventStart();
  }
  DOMTimeMilliSec LoadEventEnd() const {
    if (!nsContentUtils::IsPerformanceTimingEnabled()) {
      return 0;
    }
    return GetDOMTiming()->GetLoadEventEnd();
  }

private:
  ~nsPerformanceTiming();
  bool IsInitialized() const;
  nsRefPtr<nsPerformance> mPerformance;
  nsCOMPtr<nsITimedChannel> mChannel;
  DOMHighResTimeStamp mFetchStart;
  
  
  
  
  DOMHighResTimeStamp mZeroTime;
  bool mTimingAllowed;

  
  
  
  bool mReportCrossOriginRedirect;
};


class nsPerformanceNavigation MOZ_FINAL : public nsWrapperCache
{
public:
  explicit nsPerformanceNavigation(nsPerformance* aPerformance);
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsPerformanceNavigation)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(nsPerformanceNavigation)

  nsDOMNavigationTiming* GetDOMTiming() const;
  nsPerformanceTiming* GetPerformanceTiming() const;

  nsPerformance* GetParentObject() const
  {
    return mPerformance;
  }

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

  
  uint16_t Type() const {
    return GetDOMTiming()->GetType();
  }
  uint16_t RedirectCount() const {
    return GetPerformanceTiming()->GetRedirectCount();
  }

private:
  ~nsPerformanceNavigation();
  nsRefPtr<nsPerformance> mPerformance;
};


class nsPerformance MOZ_FINAL : public mozilla::DOMEventTargetHelper
{
public:
  typedef mozilla::dom::PerformanceEntry PerformanceEntry;
  nsPerformance(nsPIDOMWindow* aWindow,
                nsDOMNavigationTiming* aDOMTiming,
                nsITimedChannel* aChannel,
                nsPerformance* aParentPerformance);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsPerformance, DOMEventTargetHelper)

  nsDOMNavigationTiming* GetDOMTiming() const
  {
    return mDOMTiming;
  }

  nsITimedChannel* GetChannel() const
  {
    return mChannel;
  }

  nsPerformance* GetParentPerformance() const
  {
    return mParentPerformance;
  }

  nsPIDOMWindow* GetParentObject() const
  {
    return mWindow.get();
  }

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

  
  DOMHighResTimeStamp Now();
  nsPerformanceTiming* Timing();
  nsPerformanceNavigation* Navigation();

  void GetEntries(nsTArray<nsRefPtr<PerformanceEntry> >& retval);
  void GetEntriesByType(const nsAString& entryType,
                        nsTArray<nsRefPtr<PerformanceEntry> >& retval);
  void GetEntriesByName(const nsAString& name,
                        const mozilla::dom::Optional< nsAString >& entryType,
                        nsTArray<nsRefPtr<PerformanceEntry> >& retval);
  void AddEntry(nsIHttpChannel* channel,
                nsITimedChannel* timedChannel);
  void ClearResourceTimings();
  void SetResourceTimingBufferSize(uint64_t maxSize);
  IMPL_EVENT_HANDLER(resourcetimingbufferfull)

private:
  ~nsPerformance();
  void DispatchBufferFullEvent();

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<nsDOMNavigationTiming> mDOMTiming;
  nsCOMPtr<nsITimedChannel> mChannel;
  nsRefPtr<nsPerformanceTiming> mTiming;
  nsRefPtr<nsPerformanceNavigation> mNavigation;
  nsTArray<nsRefPtr<PerformanceEntry> > mEntries;
  nsRefPtr<nsPerformance> mParentPerformance;
  uint64_t mPrimaryBufferSize;

  static const uint64_t kDefaultBufferSize = 150;

  
  class PerformanceEntryComparator {
    public:
      bool Equals(const PerformanceEntry* aElem1,
                  const PerformanceEntry* aElem2) const;
      bool LessThan(const PerformanceEntry* aElem1,
                    const PerformanceEntry* aElem2) const;
  };
};

inline nsDOMNavigationTiming*
nsPerformanceNavigation::GetDOMTiming() const
{
  return mPerformance->GetDOMTiming();
}

inline nsPerformanceTiming*
nsPerformanceNavigation::GetPerformanceTiming() const
{
  return mPerformance->Timing();
}

inline nsDOMNavigationTiming*
nsPerformanceTiming::GetDOMTiming() const
{
  return mPerformance->GetDOMTiming();
}

#endif 

