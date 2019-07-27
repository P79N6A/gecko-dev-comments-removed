




#include "nsPerformance.h"
#include "nsCOMPtr.h"
#include "nsIHttpChannel.h"
#include "nsITimedChannel.h"
#include "nsDOMNavigationTiming.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDOMWindow.h"
#include "nsIURI.h"
#include "PerformanceEntry.h"
#include "PerformanceResourceTiming.h"
#include "mozilla/dom/PerformanceBinding.h"
#include "mozilla/dom/PerformanceTimingBinding.h"
#include "mozilla/dom/PerformanceNavigationBinding.h"
#include "mozilla/TimeStamp.h"
#include "nsThreadUtils.h"
#include "nsILoadInfo.h"

using namespace mozilla;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(nsPerformanceTiming, mPerformance)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsPerformanceTiming, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsPerformanceTiming, Release)

nsPerformanceTiming::nsPerformanceTiming(nsPerformance* aPerformance,
                                         nsITimedChannel* aChannel,
                                         nsIHttpChannel* aHttpChannel,
                                         DOMHighResTimeStamp aZeroTime)
  : mPerformance(aPerformance),
    mFetchStart(0.0),
    mZeroTime(aZeroTime),
    mRedirectCount(0),
    mTimingAllowed(true),
    mAllRedirectsSameOrigin(true),
    mInitialized(!!aChannel),
    mReportCrossOriginRedirect(true)
{
  MOZ_ASSERT(aPerformance, "Parent performance object should be provided");

  if (!nsContentUtils::IsPerformanceTimingEnabled()) {
    mZeroTime = 0;
  }

  
  
  
  if (aHttpChannel) {
    mTimingAllowed = CheckAllowedOrigin(aHttpChannel, aChannel);
    bool redirectsPassCheck = false;
    aChannel->GetAllRedirectsPassTimingAllowCheck(&redirectsPassCheck);
    mReportCrossOriginRedirect = mTimingAllowed && redirectsPassCheck;
  }

  InitializeTimingInfo(aChannel);
}



void
nsPerformanceTiming::InitializeTimingInfo(nsITimedChannel* aChannel)
{
  if (aChannel) {
    aChannel->GetAsyncOpen(&mAsyncOpen);
    aChannel->GetAllRedirectsSameOrigin(&mAllRedirectsSameOrigin);
    aChannel->GetRedirectCount(&mRedirectCount);
    aChannel->GetRedirectStart(&mRedirectStart);
    aChannel->GetRedirectEnd(&mRedirectEnd);
    aChannel->GetDomainLookupStart(&mDomainLookupStart);
    aChannel->GetDomainLookupEnd(&mDomainLookupEnd);
    aChannel->GetConnectStart(&mConnectStart);
    aChannel->GetConnectEnd(&mConnectEnd);
    aChannel->GetRequestStart(&mRequestStart);
    aChannel->GetResponseStart(&mResponseStart);
    aChannel->GetCacheReadStart(&mCacheReadStart);
    aChannel->GetResponseEnd(&mResponseEnd);
    aChannel->GetCacheReadEnd(&mCacheReadEnd);
  }
}

nsPerformanceTiming::~nsPerformanceTiming()
{
}

DOMHighResTimeStamp
nsPerformanceTiming::FetchStartHighRes()
{
  if (!mFetchStart) {
    if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
      return mZeroTime;
    }
    MOZ_ASSERT(!mAsyncOpen.IsNull(), "The fetch start time stamp should always be "
        "valid if the performance timing is enabled");
    mFetchStart = (!mAsyncOpen.IsNull())
        ? TimeStampToDOMHighRes(mAsyncOpen)
        : 0.0;
  }
  return mFetchStart;
}

DOMTimeMilliSec
nsPerformanceTiming::FetchStart()
{
  return static_cast<int64_t>(FetchStartHighRes());
}

bool
nsPerformanceTiming::CheckAllowedOrigin(nsIHttpChannel* aResourceChannel,
                                        nsITimedChannel* aChannel)
{
  if (!IsInitialized()) {
    return false;
  }

  
  nsCOMPtr<nsILoadInfo> loadInfo;
  aResourceChannel->GetLoadInfo(getter_AddRefs(loadInfo));
  if (!loadInfo) {
    return false;
  }
  nsCOMPtr<nsIPrincipal> principal = loadInfo->LoadingPrincipal();

  
  
  
  return aChannel->TimingAllowCheck(principal);
}

bool
nsPerformanceTiming::TimingAllowed() const
{
  return mTimingAllowed;
}

uint16_t
nsPerformanceTiming::GetRedirectCount() const
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return 0;
  }
  if (!mAllRedirectsSameOrigin) {
    return 0;
  }
  return mRedirectCount;
}

bool
nsPerformanceTiming::ShouldReportCrossOriginRedirect() const
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return false;
  }

  
  
  
  return (mRedirectCount != 0) && mReportCrossOriginRedirect;
}











DOMHighResTimeStamp
nsPerformanceTiming::RedirectStartHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  return TimeStampToDOMHighResOrFetchStart(mRedirectStart);
}

DOMTimeMilliSec
nsPerformanceTiming::RedirectStart()
{
  if (!IsInitialized()) {
    return 0;
  }
  
  
  if (mAllRedirectsSameOrigin && mRedirectCount) {
    return static_cast<int64_t>(RedirectStartHighRes());
  }
  return 0;
}











DOMHighResTimeStamp
nsPerformanceTiming::RedirectEndHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  return TimeStampToDOMHighResOrFetchStart(mRedirectEnd);
}

DOMTimeMilliSec
nsPerformanceTiming::RedirectEnd()
{
  if (!IsInitialized()) {
    return 0;
  }
  
  
  if (mAllRedirectsSameOrigin && mRedirectCount) {
    return static_cast<int64_t>(RedirectEndHighRes());
  }
  return 0;
}

DOMHighResTimeStamp
nsPerformanceTiming::DomainLookupStartHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  return TimeStampToDOMHighResOrFetchStart(mDomainLookupStart);
}

DOMTimeMilliSec
nsPerformanceTiming::DomainLookupStart()
{
  return static_cast<int64_t>(DomainLookupStartHighRes());
}

DOMHighResTimeStamp
nsPerformanceTiming::DomainLookupEndHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  return TimeStampToDOMHighResOrFetchStart(mDomainLookupEnd);
}

DOMTimeMilliSec
nsPerformanceTiming::DomainLookupEnd()
{
  return static_cast<int64_t>(DomainLookupEndHighRes());
}

DOMHighResTimeStamp
nsPerformanceTiming::ConnectStartHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  return TimeStampToDOMHighResOrFetchStart(mConnectStart);
}

DOMTimeMilliSec
nsPerformanceTiming::ConnectStart()
{
  return static_cast<int64_t>(ConnectStartHighRes());
}

DOMHighResTimeStamp
nsPerformanceTiming::ConnectEndHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  return TimeStampToDOMHighResOrFetchStart(mConnectEnd);
}

DOMTimeMilliSec
nsPerformanceTiming::ConnectEnd()
{
  return static_cast<int64_t>(ConnectEndHighRes());
}

DOMHighResTimeStamp
nsPerformanceTiming::RequestStartHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  return TimeStampToDOMHighResOrFetchStart(mRequestStart);
}

DOMTimeMilliSec
nsPerformanceTiming::RequestStart()
{
  return static_cast<int64_t>(RequestStartHighRes());
}

DOMHighResTimeStamp
nsPerformanceTiming::ResponseStartHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  if (mResponseStart.IsNull() ||
     (!mCacheReadStart.IsNull() && mCacheReadStart < mResponseStart)) {
    mResponseStart = mCacheReadStart;
  }
  return TimeStampToDOMHighResOrFetchStart(mResponseStart);
}

DOMTimeMilliSec
nsPerformanceTiming::ResponseStart()
{
  return static_cast<int64_t>(ResponseStartHighRes());
}

DOMHighResTimeStamp
nsPerformanceTiming::ResponseEndHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return mZeroTime;
  }
  if (mResponseEnd.IsNull() ||
     (!mCacheReadEnd.IsNull() && mCacheReadEnd < mResponseEnd)) {
    mResponseEnd = mCacheReadEnd;
  }
  return TimeStampToDOMHighResOrFetchStart(mResponseEnd);
}

DOMTimeMilliSec
nsPerformanceTiming::ResponseEnd()
{
  return static_cast<int64_t>(ResponseEndHighRes());
}

bool
nsPerformanceTiming::IsInitialized() const
{
  return mInitialized;
}

JSObject*
nsPerformanceTiming::WrapObject(JSContext *cx)
{
  return dom::PerformanceTimingBinding::Wrap(cx, this);
}


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(nsPerformanceNavigation, mPerformance)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsPerformanceNavigation, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsPerformanceNavigation, Release)

nsPerformanceNavigation::nsPerformanceNavigation(nsPerformance* aPerformance)
  : mPerformance(aPerformance)
{
  MOZ_ASSERT(aPerformance, "Parent performance object should be provided");
}

nsPerformanceNavigation::~nsPerformanceNavigation()
{
}

JSObject*
nsPerformanceNavigation::WrapObject(JSContext *cx)
{
  return dom::PerformanceNavigationBinding::Wrap(cx, this);
}


NS_IMPL_CYCLE_COLLECTION_INHERITED(nsPerformance, DOMEventTargetHelper,
                                   mWindow, mTiming,
                                   mNavigation, mEntries,
                                   mParentPerformance)
NS_IMPL_ADDREF_INHERITED(nsPerformance, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsPerformance, DOMEventTargetHelper)

nsPerformance::nsPerformance(nsPIDOMWindow* aWindow,
                             nsDOMNavigationTiming* aDOMTiming,
                             nsITimedChannel* aChannel,
                             nsPerformance* aParentPerformance)
  : DOMEventTargetHelper(aWindow),
    mWindow(aWindow),
    mDOMTiming(aDOMTiming),
    mChannel(aChannel),
    mParentPerformance(aParentPerformance),
    mPrimaryBufferSize(kDefaultBufferSize)
{
  MOZ_ASSERT(aWindow, "Parent window object should be provided");
}

nsPerformance::~nsPerformance()
{
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsPerformance)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)


nsPerformanceTiming*
nsPerformance::Timing()
{
  if (!mTiming) {
    
    
    
    
    mTiming = new nsPerformanceTiming(this, mChannel, nullptr,
        mDOMTiming->GetNavigationStart());
  }
  return mTiming;
}

void
nsPerformance::DispatchBufferFullEvent()
{
  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
  if (NS_SUCCEEDED(rv)) {
    
    rv = event->InitEvent(NS_LITERAL_STRING("resourcetimingbufferfull"), true, false);
    if (NS_SUCCEEDED(rv)) {
      event->SetTrusted(true);
      DispatchDOMEvent(nullptr, event, nullptr, nullptr);
    }
  }
}

nsPerformanceNavigation*
nsPerformance::Navigation()
{
  if (!mNavigation) {
    mNavigation = new nsPerformanceNavigation(this);
  }
  return mNavigation;
}

DOMHighResTimeStamp
nsPerformance::Now()
{
  return GetDOMTiming()->TimeStampToDOMHighRes(mozilla::TimeStamp::Now());
}

JSObject*
nsPerformance::WrapObject(JSContext *cx)
{
  return dom::PerformanceBinding::Wrap(cx, this);
}

void
nsPerformance::GetEntries(nsTArray<nsRefPtr<PerformanceEntry> >& retval)
{
  MOZ_ASSERT(NS_IsMainThread());

  retval = mEntries;
}

void
nsPerformance::GetEntriesByType(const nsAString& entryType,
                                nsTArray<nsRefPtr<PerformanceEntry> >& retval)
{
  MOZ_ASSERT(NS_IsMainThread());

  retval.Clear();
  uint32_t count = mEntries.Length();
  for (uint32_t i = 0 ; i < count; i++) {
    if (mEntries[i]->GetEntryType().Equals(entryType)) {
      retval.AppendElement(mEntries[i]);
    }
  }
}

void
nsPerformance::GetEntriesByName(const nsAString& name,
                                const mozilla::dom::Optional<nsAString>& entryType,
                                nsTArray<nsRefPtr<PerformanceEntry> >& retval)
{
  MOZ_ASSERT(NS_IsMainThread());

  retval.Clear();
  uint32_t count = mEntries.Length();
  for (uint32_t i = 0 ; i < count; i++) {
    if (mEntries[i]->GetName().Equals(name) &&
        (!entryType.WasPassed() ||
         mEntries[i]->GetEntryType().Equals(entryType.Value()))) {
      retval.AppendElement(mEntries[i]);
    }
  }
}

void
nsPerformance::ClearResourceTimings()
{
  MOZ_ASSERT(NS_IsMainThread());
  mEntries.Clear();
}

void
nsPerformance::SetResourceTimingBufferSize(uint64_t maxSize)
{
  MOZ_ASSERT(NS_IsMainThread());
  mPrimaryBufferSize = maxSize;
}





void
nsPerformance::AddEntry(nsIHttpChannel* channel,
                        nsITimedChannel* timedChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  if (!nsContentUtils::IsResourceTimingEnabled()) {
    return;
  }

  
  if (mEntries.Length() >= mPrimaryBufferSize) {
    return;
  }

  if (channel && timedChannel) {
    nsAutoCString name;
    nsAutoString initiatorType;
    nsCOMPtr<nsIURI> originalURI;

    timedChannel->GetInitiatorType(initiatorType);

    
    
    
    channel->GetOriginalURI(getter_AddRefs(originalURI));
    originalURI->GetSpec(name);
    NS_ConvertUTF8toUTF16 entryName(name);

    
    
    
    
    
    
    nsRefPtr<nsPerformanceTiming> performanceTiming =
        new nsPerformanceTiming(this, timedChannel, channel,
            0);

    
    
    nsRefPtr<dom::PerformanceResourceTiming> performanceEntry =
        new dom::PerformanceResourceTiming(performanceTiming, this);

    performanceEntry->SetName(entryName);
    performanceEntry->SetEntryType(NS_LITERAL_STRING("resource"));
    
    
    if (initiatorType.IsEmpty()) {
      initiatorType = NS_LITERAL_STRING("other");
    }
    performanceEntry->SetInitiatorType(initiatorType);

    mEntries.InsertElementSorted(performanceEntry,
        PerformanceEntryComparator());
    if (mEntries.Length() >= mPrimaryBufferSize) {
      
      DispatchBufferFullEvent();
    }
  }
}

bool
nsPerformance::PerformanceEntryComparator::Equals(
    const PerformanceEntry* aElem1,
    const PerformanceEntry* aElem2) const
{
  NS_ABORT_IF_FALSE(aElem1 && aElem2,
      "Trying to compare null performance entries");
  return aElem1->StartTime() == aElem2->StartTime();
}

bool
nsPerformance::PerformanceEntryComparator::LessThan(
    const PerformanceEntry* aElem1,
    const PerformanceEntry* aElem2) const
{
  NS_ABORT_IF_FALSE(aElem1 && aElem2,
      "Trying to compare null performance entries");
  return aElem1->StartTime() < aElem2->StartTime();
}
