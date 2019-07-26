




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

using namespace mozilla;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(nsPerformanceTiming, mPerformance)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsPerformanceTiming, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsPerformanceTiming, Release)

nsPerformanceTiming::nsPerformanceTiming(nsPerformance* aPerformance,
                                         nsITimedChannel* aChannel,
                                         nsIHttpChannel* aHttpChannel,
                                         DOMHighResTimeStamp aZeroTime)
  : mPerformance(aPerformance),
    mChannel(aChannel),
    mFetchStart(0.0),
    mZeroTime(aZeroTime),
    mReportCrossOriginResources(true)
{
  MOZ_ASSERT(aPerformance, "Parent performance object should be provided");
  SetIsDOMBinding();
  
  
  
  if (aHttpChannel) {
    CheckRedirectCrossOrigin(aHttpChannel);
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
      return 0;
    }
    TimeStamp stamp;
    mChannel->GetAsyncOpen(&stamp);
    MOZ_ASSERT(!stamp.IsNull(), "The fetch start time stamp should always be "
        "valid if the performance timing is enabled");
    mFetchStart = (!stamp.IsNull())
        ? TimeStampToDOMHighRes(stamp)
        : 0.0;
  }
  return mFetchStart;
}

DOMTimeMilliSec
nsPerformanceTiming::FetchStart()
{
  return static_cast<int64_t>(FetchStartHighRes());
}




void
nsPerformanceTiming::CheckRedirectCrossOrigin(nsIHttpChannel* aResourceChannel)
{
  MOZ_ASSERT(mChannel, "The resource channel should not be null for any"
            "valid resource");
  uint16_t redirectCount;
  mChannel->GetRedirectCount(&redirectCount);
  if (redirectCount == 0) {
    return;
  }
  nsCOMPtr<nsIURI> resourceURI, referrerURI;
  aResourceChannel->GetReferrer(getter_AddRefs(referrerURI));
  aResourceChannel->GetURI(getter_AddRefs(resourceURI));
  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  nsresult rv = ssm->CheckSameOriginURI(resourceURI, referrerURI, false);
  if (!NS_SUCCEEDED(rv)) {
    mReportCrossOriginResources = false;
  }
}

bool
nsPerformanceTiming::IsSameOriginAsReferral() const
{
  return mReportCrossOriginResources;
}

uint16_t
nsPerformanceTiming::GetRedirectCount() const
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return 0;
  }
  bool sameOrigin;
  mChannel->GetAllRedirectsSameOrigin(&sameOrigin);
  if (!sameOrigin) {
    return 0;
  }
  uint16_t redirectCount;
  mChannel->GetRedirectCount(&redirectCount);
  return redirectCount;
}











DOMHighResTimeStamp
nsPerformanceTiming::RedirectStartHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetRedirectStart(&stamp);
  return TimeStampToDOMHighResOrFetchStart(stamp);
}

DOMTimeMilliSec
nsPerformanceTiming::RedirectStart()
{
  
  
  bool sameOrigin;
  mChannel->GetAllRedirectsSameOrigin(&sameOrigin);
  if (sameOrigin) {
    return static_cast<int64_t>(RedirectStartHighRes());
  }
  return 0;
}











DOMHighResTimeStamp
nsPerformanceTiming::RedirectEndHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetRedirectEnd(&stamp);
  return TimeStampToDOMHighResOrFetchStart(stamp);
}

DOMTimeMilliSec
nsPerformanceTiming::RedirectEnd()
{
  
  
  bool sameOrigin;
  mChannel->GetAllRedirectsSameOrigin(&sameOrigin);
  if (sameOrigin) {
    return static_cast<int64_t>(RedirectEndHighRes());
  }
  return 0;
}

DOMHighResTimeStamp
nsPerformanceTiming::DomainLookupStartHighRes()
{
  if (!nsContentUtils::IsPerformanceTimingEnabled() || !IsInitialized()) {
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetDomainLookupStart(&stamp);
  return TimeStampToDOMHighResOrFetchStart(stamp);
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
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetDomainLookupEnd(&stamp);
  return TimeStampToDOMHighResOrFetchStart(stamp);
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
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetConnectStart(&stamp);
  return TimeStampToDOMHighResOrFetchStart(stamp);
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
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetConnectEnd(&stamp);
  return TimeStampToDOMHighResOrFetchStart(stamp);
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
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetRequestStart(&stamp);
  return TimeStampToDOMHighResOrFetchStart(stamp);
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
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetResponseStart(&stamp);
  mozilla::TimeStamp cacheStamp;
  mChannel->GetCacheReadStart(&cacheStamp);
  if (stamp.IsNull() || (!cacheStamp.IsNull() && cacheStamp < stamp)) {
    stamp = cacheStamp;
  }
  return TimeStampToDOMHighResOrFetchStart(stamp);
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
    return 0;
  }
  mozilla::TimeStamp stamp;
  mChannel->GetResponseEnd(&stamp);
  mozilla::TimeStamp cacheStamp;
  mChannel->GetCacheReadEnd(&cacheStamp);
  if (stamp.IsNull() || (!cacheStamp.IsNull() && cacheStamp < stamp)) {
    stamp = cacheStamp;
  }
  return TimeStampToDOMHighResOrFetchStart(stamp);
}

DOMTimeMilliSec
nsPerformanceTiming::ResponseEnd()
{
  return static_cast<int64_t>(ResponseEndHighRes());
}

bool
nsPerformanceTiming::IsInitialized() const
{
  MOZ_ASSERT(mChannel, "The timed channel should not be null for any "
      "valid resource");
  return !!mChannel;
}

JSObject*
nsPerformanceTiming::WrapObject(JSContext *cx)
{
  return dom::PerformanceTimingBinding::Wrap(cx, this);
}


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(nsPerformanceNavigation, mPerformance)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsPerformanceNavigation, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsPerformanceNavigation, Release)

nsPerformanceNavigation::nsPerformanceNavigation(nsPerformance* aPerformance)
  : mPerformance(aPerformance)
{
  MOZ_ASSERT(aPerformance, "Parent performance object should be provided");
  SetIsDOMBinding();
}

nsPerformanceNavigation::~nsPerformanceNavigation()
{
}

JSObject*
nsPerformanceNavigation::WrapObject(JSContext *cx)
{
  return dom::PerformanceNavigationBinding::Wrap(cx, this);
}


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_5(nsPerformance,
                                        mWindow, mTiming,
                                        mNavigation, mEntries,
                                        mParentPerformance)
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsPerformance)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsPerformance)

nsPerformance::nsPerformance(nsIDOMWindow* aWindow,
                             nsDOMNavigationTiming* aDOMTiming,
                             nsITimedChannel* aChannel,
                             nsPerformance* aParentPerformance)
  : mWindow(aWindow),
    mDOMTiming(aDOMTiming),
    mChannel(aChannel),
    mParentPerformance(aParentPerformance),
    mBufferSizeSet(kDefaultBufferSize),
    mPrimaryBufferSize(kDefaultBufferSize)
{
  MOZ_ASSERT(aWindow, "Parent window object should be provided");
  SetIsDOMBinding();
}

nsPerformance::~nsPerformance()
{
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsPerformance)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


nsPerformanceTiming*
nsPerformance::Timing()
{
  if (!mTiming) {
    
    
    
    
    mTiming = new nsPerformanceTiming(this, mChannel, nullptr,
        mDOMTiming->GetNavigationStart());
  }
  return mTiming;
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

  retval.Clear();
  uint32_t count = mEntries.Length();
  if (count > mPrimaryBufferSize) {
    count = mPrimaryBufferSize;
  }
  retval.AppendElements(mEntries.Elements(), count);
}

void
nsPerformance::GetEntriesByType(const nsAString& entryType,
                                nsTArray<nsRefPtr<PerformanceEntry> >& retval)
{
  MOZ_ASSERT(NS_IsMainThread());

  retval.Clear();
  uint32_t count = mEntries.Length();
  for (uint32_t i = 0 ; i < count && i < mPrimaryBufferSize ; i++) {
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
  for (uint32_t i = 0 ; i < count && i < mPrimaryBufferSize ; i++) {
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
  mPrimaryBufferSize = mBufferSizeSet;
  mEntries.Clear();
}

void
nsPerformance::SetResourceTimingBufferSize(uint64_t maxSize)
{
  MOZ_ASSERT(NS_IsMainThread());
  mBufferSizeSet = maxSize;
  if (mBufferSizeSet < mEntries.Length()) {
    
    
  }
}





void
nsPerformance::AddEntry(nsIHttpChannel* channel,
                        nsITimedChannel* timedChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  if (!nsContentUtils::IsResourceTimingEnabled()) {
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
    if (mEntries.Length() > mPrimaryBufferSize) {
      
      
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
