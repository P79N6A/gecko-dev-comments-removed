




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
#include "js/RootingAPI.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/DOMEventTargetHelper.h"

class nsITimedChannel;
class nsPerformance;
class nsIHttpChannel;

namespace mozilla {
class ErrorResult;
namespace dom {
  class PerformanceEntry;
} 
} 


class nsPerformanceTiming final : public nsWrapperCache
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

  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

  
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
  
  
  
  bool CheckAllowedOrigin(nsIHttpChannel* aResourceChannel, nsITimedChannel* aChannel);
  
  
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
  void InitializeTimingInfo(nsITimedChannel* aChannel);
  nsRefPtr<nsPerformance> mPerformance;
  DOMHighResTimeStamp mFetchStart;
  
  
  
  
  DOMHighResTimeStamp mZeroTime;

  TimeStamp mAsyncOpen;
  TimeStamp mRedirectStart;
  TimeStamp mRedirectEnd;
  TimeStamp mDomainLookupStart;
  TimeStamp mDomainLookupEnd;
  TimeStamp mConnectStart;
  TimeStamp mConnectEnd;
  TimeStamp mRequestStart;
  TimeStamp mResponseStart;
  TimeStamp mCacheReadStart;
  TimeStamp mResponseEnd;
  TimeStamp mCacheReadEnd;
  uint16_t mRedirectCount;
  bool mTimingAllowed;
  bool mAllRedirectsSameOrigin;
  bool mInitialized;

  
  
  
  bool mReportCrossOriginRedirect;
};


class nsPerformanceNavigation final : public nsWrapperCache
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

  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

  
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


class PerformanceBase : public mozilla::DOMEventTargetHelper 
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(PerformanceBase,
                                           DOMEventTargetHelper)

  PerformanceBase();
  explicit PerformanceBase(nsPIDOMWindow* aWindow);

  typedef mozilla::dom::PerformanceEntry PerformanceEntry;

  void GetEntries(nsTArray<nsRefPtr<PerformanceEntry>>& aRetval);
  void GetEntriesByType(const nsAString& aEntryType,
                        nsTArray<nsRefPtr<PerformanceEntry>>& aRetval);
  void GetEntriesByName(const nsAString& aName,
                        const mozilla::dom::Optional<nsAString>& aEntryType,
                        nsTArray<nsRefPtr<PerformanceEntry>>& aRetval);
  void ClearResourceTimings();

  virtual DOMHighResTimeStamp Now() const = 0;

  void Mark(const nsAString& aName, mozilla::ErrorResult& aRv);
  void ClearMarks(const mozilla::dom::Optional<nsAString>& aName);
  void Measure(const nsAString& aName,
               const mozilla::dom::Optional<nsAString>& aStartMark,
               const mozilla::dom::Optional<nsAString>& aEndMark,
               mozilla::ErrorResult& aRv);
  void ClearMeasures(const mozilla::dom::Optional<nsAString>& aName);

  void SetResourceTimingBufferSize(uint64_t aMaxSize);

protected:
  virtual ~PerformanceBase();

  virtual void InsertUserEntry(PerformanceEntry* aEntry);
  void InsertResourceEntry(PerformanceEntry* aEntry);

  void ClearUserEntries(const mozilla::dom::Optional<nsAString>& aEntryName,
                        const nsAString& aEntryType);

  DOMHighResTimeStamp ResolveTimestampFromName(const nsAString& aName,
                                               mozilla::ErrorResult& aRv);

  virtual nsISupports* GetAsISupports() = 0;

  virtual void DispatchBufferFullEvent() = 0;

  virtual DOMHighResTimeStamp
  DeltaFromNavigationStart(DOMHighResTimeStamp aTime) = 0;

  virtual bool IsPerformanceTimingAttribute(const nsAString& aName) = 0;

  virtual DOMHighResTimeStamp
  GetPerformanceTimingFromString(const nsAString& aTimingName) = 0;

  bool IsResourceEntryLimitReached() const
  {
    return mResourceEntries.Length() >= mResourceTimingBufferSize;
  }

private:
  nsTArray<nsRefPtr<PerformanceEntry>> mUserEntries;
  nsTArray<nsRefPtr<PerformanceEntry>> mResourceEntries;

  uint64_t mResourceTimingBufferSize;
  static const uint64_t kDefaultResourceTimingBufferSize = 150;
};


class nsPerformance final : public PerformanceBase
{
public:
  nsPerformance(nsPIDOMWindow* aWindow,
                nsDOMNavigationTiming* aDOMTiming,
                nsITimedChannel* aChannel,
                nsPerformance* aParentPerformance);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsPerformance,
                                                         PerformanceBase)

  static bool IsEnabled(JSContext* aCx, JSObject* aGlobal);

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

  JSObject* WrapObject(JSContext *cx,
                       JS::Handle<JSObject*> aGivenProto) override;

  
  DOMHighResTimeStamp Now() const override;

  nsPerformanceTiming* Timing();
  nsPerformanceNavigation* Navigation();

  void AddEntry(nsIHttpChannel* channel,
                nsITimedChannel* timedChannel);

  using PerformanceBase::GetEntries;
  using PerformanceBase::GetEntriesByType;
  using PerformanceBase::GetEntriesByName;
  using PerformanceBase::ClearResourceTimings;

  using PerformanceBase::Mark;
  using PerformanceBase::ClearMarks;
  using PerformanceBase::Measure;
  using PerformanceBase::ClearMeasures;
  using PerformanceBase::SetResourceTimingBufferSize;

  void GetMozMemory(JSContext *aCx, JS::MutableHandle<JSObject*> aObj);

  IMPL_EVENT_HANDLER(resourcetimingbufferfull)

private:
  ~nsPerformance();

  nsISupports* GetAsISupports() override
  {
    return this;
  }

  void InsertUserEntry(PerformanceEntry* aEntry) override;

  bool IsPerformanceTimingAttribute(const nsAString& aName) override;

  DOMHighResTimeStamp
  DeltaFromNavigationStart(DOMHighResTimeStamp aTime) override;

  DOMHighResTimeStamp
  GetPerformanceTimingFromString(const nsAString& aTimingName) override;

  void DispatchBufferFullEvent() override;

  nsRefPtr<nsDOMNavigationTiming> mDOMTiming;
  nsCOMPtr<nsITimedChannel> mChannel;
  nsRefPtr<nsPerformanceTiming> mTiming;
  nsRefPtr<nsPerformanceNavigation> mNavigation;
  nsRefPtr<nsPerformance> mParentPerformance;
  JS::Heap<JSObject*> mMozMemory;
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

