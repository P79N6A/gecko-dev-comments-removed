





#include "nsPerformance.h"
#include "nsCOMPtr.h"
#include "nsIHttpChannel.h"
#include "nsITimedChannel.h"
#include "nsDOMNavigationTiming.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDOMWindow.h"
#include "nsILoadInfo.h"
#include "nsIURI.h"
#include "nsThreadUtils.h"
#include "PerformanceEntry.h"
#include "PerformanceMark.h"
#include "PerformanceMeasure.h"
#include "PerformanceResourceTiming.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/PerformanceBinding.h"
#include "mozilla/dom/PerformanceTimingBinding.h"
#include "mozilla/dom/PerformanceNavigationBinding.h"
#include "mozilla/Preferences.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/TimeStamp.h"
#include "js/HeapAPI.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"

#ifdef MOZ_WIDGET_GONK
#define PERFLOG(msg, ...)  __android_log_print(ANDROID_LOG_INFO, "PerformanceTiming", msg, ##__VA_ARGS__)
#else
#define PERFLOG(msg, ...) printf_stderr(msg, ##__VA_ARGS__)
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::workers;

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
  
  return mDomainLookupEnd.IsNull() ? DomainLookupStartHighRes()
                                   : TimeStampToDOMHighRes(mDomainLookupEnd);
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
  return mConnectStart.IsNull() ? DomainLookupEndHighRes()
                                : TimeStampToDOMHighRes(mConnectStart);
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
  
  return mConnectEnd.IsNull() ? ConnectStartHighRes()
                              : TimeStampToDOMHighRes(mConnectEnd);
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
  if (!IsInitialized()) {
    return mZeroTime;
  }
  if (mResponseEnd.IsNull() ||
     (!mCacheReadEnd.IsNull() && mCacheReadEnd < mResponseEnd)) {
    mResponseEnd = mCacheReadEnd;
  }
  
  return mResponseEnd.IsNull() ? ResponseStartHighRes()
                               : TimeStampToDOMHighRes(mResponseEnd);
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
nsPerformanceTiming::WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto)
{
  return PerformanceTimingBinding::Wrap(cx, this, aGivenProto);
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
nsPerformanceNavigation::WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto)
{
  return PerformanceNavigationBinding::Wrap(cx, this, aGivenProto);
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsPerformance)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsPerformance, PerformanceBase)
NS_IMPL_CYCLE_COLLECTION_UNLINK(mTiming,
                                mNavigation,
                                mParentPerformance)
  tmp->mMozMemory = nullptr;
  mozilla::DropJSObjects(this);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsPerformance, PerformanceBase)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTiming,
                                    mNavigation,
                                    mParentPerformance)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(nsPerformance, PerformanceBase)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mMozMemory)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_ADDREF_INHERITED(nsPerformance, PerformanceBase)
NS_IMPL_RELEASE_INHERITED(nsPerformance, PerformanceBase)

nsPerformance::nsPerformance(nsPIDOMWindow* aWindow,
                             nsDOMNavigationTiming* aDOMTiming,
                             nsITimedChannel* aChannel,
                             nsPerformance* aParentPerformance)
  : PerformanceBase(aWindow),
    mDOMTiming(aDOMTiming),
    mChannel(aChannel),
    mParentPerformance(aParentPerformance)
{
  MOZ_ASSERT(aWindow, "Parent window object should be provided");
}

nsPerformance::~nsPerformance()
{
  mozilla::DropJSObjects(this);
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsPerformance)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

void
nsPerformance::GetMozMemory(JSContext *aCx, JS::MutableHandle<JSObject*> aObj)
{
  if (!mMozMemory) {
    mMozMemory = js::gc::NewMemoryInfoObject(aCx);
    if (mMozMemory) {
      mozilla::HoldJSObjects(this);
    }
  }

  aObj.set(mMozMemory);
}

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
nsPerformance::Now() const
{
  double nowTimeMs = GetDOMTiming()->TimeStampToDOMHighRes(TimeStamp::Now());
  
  
  const double maxResolutionMs = 0.005;
  return floor(nowTimeMs / maxResolutionMs) * maxResolutionMs;
}

JSObject*
nsPerformance::WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto)
{
  return PerformanceBinding::Wrap(cx, this, aGivenProto);
}





void
nsPerformance::AddEntry(nsIHttpChannel* channel,
                        nsITimedChannel* timedChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  if (!nsContentUtils::IsResourceTimingEnabled()) {
    return;
  }

  
  if (IsResourceEntryLimitReached()) {
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

    
    
    nsRefPtr<PerformanceResourceTiming> performanceEntry =
      new PerformanceResourceTiming(performanceTiming, this, entryName);

    
    
    if (initiatorType.IsEmpty()) {
      initiatorType = NS_LITERAL_STRING("other");
    }
    performanceEntry->SetInitiatorType(initiatorType);
    InsertResourceEntry(performanceEntry);
  }
}


bool
nsPerformance::IsPerformanceTimingAttribute(const nsAString& aName)
{
  
  static const char* attributes[] =
    {"navigationStart", "unloadEventStart", "unloadEventEnd", "redirectStart",
     "redirectEnd", "fetchStart", "domainLookupStart", "domainLookupEnd",
     "connectStart", "connectEnd", "requestStart", "responseStart",
     "responseEnd", "domLoading", "domInteractive", "domContentLoadedEventStart",
     "domContentLoadedEventEnd", "domComplete", "loadEventStart",
     "loadEventEnd", nullptr};

  for (uint32_t i = 0; attributes[i]; ++i) {
    if (aName.EqualsASCII(attributes[i])) {
      return true;
    }
  }
  return false;
}

DOMHighResTimeStamp
nsPerformance::GetPerformanceTimingFromString(const nsAString& aProperty)
{
  if (!IsPerformanceTimingAttribute(aProperty)) {
    return 0;
  }
  if (aProperty.EqualsLiteral("navigationStart")) {
    
    
    return GetDOMTiming()->GetNavigationStart();
  }
  if (aProperty.EqualsLiteral("unloadEventStart")) {
    return GetDOMTiming()->GetUnloadEventStart();
  }
  if (aProperty.EqualsLiteral("unloadEventEnd")) {
    return GetDOMTiming()->GetUnloadEventEnd();
  }
  if (aProperty.EqualsLiteral("redirectStart")) {
    return Timing()->RedirectStart();
  }
  if (aProperty.EqualsLiteral("redirectEnd")) {
    return Timing()->RedirectEnd();
  }
  if (aProperty.EqualsLiteral("fetchStart")) {
    return Timing()->FetchStart();
  }
  if (aProperty.EqualsLiteral("domainLookupStart")) {
    return Timing()->DomainLookupStart();
  }
  if (aProperty.EqualsLiteral("domainLookupEnd")) {
    return Timing()->DomainLookupEnd();
  }
  if (aProperty.EqualsLiteral("connectStart")) {
    return Timing()->ConnectStart();
  }
  if (aProperty.EqualsLiteral("connectEnd")) {
    return Timing()->ConnectEnd();
  }
  if (aProperty.EqualsLiteral("requestStart")) {
    return Timing()->RequestStart();
  }
  if (aProperty.EqualsLiteral("responseStart")) {
    return Timing()->ResponseStart();
  }
  if (aProperty.EqualsLiteral("responseEnd")) {
    return Timing()->ResponseEnd();
  }
  if (aProperty.EqualsLiteral("domLoading")) {
    return GetDOMTiming()->GetDomLoading();
  }
  if (aProperty.EqualsLiteral("domInteractive")) {
    return GetDOMTiming()->GetDomInteractive();
  }
  if (aProperty.EqualsLiteral("domContentLoadedEventStart")) {
    return GetDOMTiming()->GetDomContentLoadedEventStart();
  }
  if (aProperty.EqualsLiteral("domContentLoadedEventEnd")) {
    return GetDOMTiming()->GetDomContentLoadedEventEnd();
  }
  if (aProperty.EqualsLiteral("domComplete")) {
    return GetDOMTiming()->GetDomComplete();
  }
  if (aProperty.EqualsLiteral("loadEventStart")) {
    return GetDOMTiming()->GetLoadEventStart();
  }
  if (aProperty.EqualsLiteral("loadEventEnd"))  {
    return GetDOMTiming()->GetLoadEventEnd();
  }
  MOZ_CRASH("IsPerformanceTimingAttribute and GetPerformanceTimingFromString are out of sync");
  return 0;
}

namespace {


class MOZ_STACK_CLASS PerformanceEntryComparator final
{
public:
  bool Equals(const PerformanceEntry* aElem1,
              const PerformanceEntry* aElem2) const
  {
    MOZ_ASSERT(aElem1 && aElem2,
               "Trying to compare null performance entries");
    return aElem1->StartTime() == aElem2->StartTime();
  }

  bool LessThan(const PerformanceEntry* aElem1,
                const PerformanceEntry* aElem2) const
  {
    MOZ_ASSERT(aElem1 && aElem2,
               "Trying to compare null performance entries");
    return aElem1->StartTime() < aElem2->StartTime();
  }
};

class PrefEnabledRunnable final : public WorkerMainThreadRunnable
{
public:
  explicit PrefEnabledRunnable(WorkerPrivate* aWorkerPrivate)
    : WorkerMainThreadRunnable(aWorkerPrivate)
    , mEnabled(false)
  { }

  bool MainThreadRun() override
  {
    MOZ_ASSERT(NS_IsMainThread());
    mEnabled = Preferences::GetBool("dom.enable_user_timing", false);
    return true;
  }

  bool IsEnabled() const
  {
    return mEnabled;
  }

private:
  bool mEnabled;
};

} 

 bool
nsPerformance::IsEnabled(JSContext* aCx, JSObject* aGlobal)
{
  if (NS_IsMainThread()) {
    return Preferences::GetBool("dom.enable_user_timing", false);
  }

  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);
  workerPrivate->AssertIsOnWorkerThread();

  nsRefPtr<PrefEnabledRunnable> runnable =
    new PrefEnabledRunnable(workerPrivate);
  runnable->Dispatch(workerPrivate->GetJSContext());

  return runnable->IsEnabled();
}

void
nsPerformance::InsertUserEntry(PerformanceEntry* aEntry)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (nsContentUtils::IsUserTimingLoggingEnabled()) {
    nsAutoCString uri;
    nsresult rv = GetOwner()->GetDocumentURI()->GetHost(uri);
    if(NS_FAILED(rv)) {
      
      uri.AssignLiteral("none");
    }
    PERFLOG("Performance Entry: %s|%s|%s|%f|%f|%" PRIu64 "\n",
            uri.get(),
            NS_ConvertUTF16toUTF8(aEntry->GetEntryType()).get(),
            NS_ConvertUTF16toUTF8(aEntry->GetName()).get(),
            aEntry->StartTime(),
            aEntry->Duration(),
            static_cast<uint64_t>(PR_Now() / PR_USEC_PER_MSEC));
  }

  PerformanceBase::InsertUserEntry(aEntry);
}

DOMHighResTimeStamp
nsPerformance::DeltaFromNavigationStart(DOMHighResTimeStamp aTime)
{
  
  
  if (aTime == 0) {
    return 0;
  }
  return aTime - GetDOMTiming()->GetNavigationStart();
}



NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(PerformanceBase)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_CYCLE_COLLECTION_INHERITED(PerformanceBase,
                                   DOMEventTargetHelper,
                                   mUserEntries,
                                   mResourceEntries);

NS_IMPL_ADDREF_INHERITED(PerformanceBase, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(PerformanceBase, DOMEventTargetHelper)

PerformanceBase::PerformanceBase()
  : mResourceTimingBufferSize(kDefaultResourceTimingBufferSize)
{
  MOZ_ASSERT(!NS_IsMainThread());
}

PerformanceBase::PerformanceBase(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mResourceTimingBufferSize(kDefaultResourceTimingBufferSize)
{
  MOZ_ASSERT(NS_IsMainThread());
}

PerformanceBase::~PerformanceBase()
{}

void
PerformanceBase::GetEntries(nsTArray<nsRefPtr<PerformanceEntry>>& aRetval)
{
  aRetval = mResourceEntries;
  aRetval.AppendElements(mUserEntries);
  aRetval.Sort(PerformanceEntryComparator());
}

void
PerformanceBase::GetEntriesByType(const nsAString& aEntryType,
                                  nsTArray<nsRefPtr<PerformanceEntry>>& aRetval)
{
  if (aEntryType.EqualsLiteral("resource")) {
    aRetval = mResourceEntries;
    return;
  }

  aRetval.Clear();

  if (aEntryType.EqualsLiteral("mark") ||
      aEntryType.EqualsLiteral("measure")) {
    for (PerformanceEntry* entry : mUserEntries) {
      if (entry->GetEntryType().Equals(aEntryType)) {
        aRetval.AppendElement(entry);
      }
    }
  }
}

void
PerformanceBase::GetEntriesByName(const nsAString& aName,
                                  const Optional<nsAString>& aEntryType,
                                  nsTArray<nsRefPtr<PerformanceEntry>>& aRetval)
{
  aRetval.Clear();

  for (PerformanceEntry* entry : mResourceEntries) {
    if (entry->GetName().Equals(aName) &&
        (!aEntryType.WasPassed() ||
         entry->GetEntryType().Equals(aEntryType.Value()))) {
      aRetval.AppendElement(entry);
    }
  }

  for (PerformanceEntry* entry : mUserEntries) {
    if (entry->GetName().Equals(aName) &&
        (!aEntryType.WasPassed() ||
         entry->GetEntryType().Equals(aEntryType.Value()))) {
      aRetval.AppendElement(entry);
    }
  }

  aRetval.Sort(PerformanceEntryComparator());
}

void
PerformanceBase::ClearUserEntries(const Optional<nsAString>& aEntryName,
                                  const nsAString& aEntryType)
{
  for (uint32_t i = 0; i < mUserEntries.Length();) {
    if ((!aEntryName.WasPassed() ||
         mUserEntries[i]->GetName().Equals(aEntryName.Value())) &&
        (aEntryType.IsEmpty() ||
         mUserEntries[i]->GetEntryType().Equals(aEntryType))) {
      mUserEntries.RemoveElementAt(i);
    } else {
      ++i;
    }
  }
}

void
PerformanceBase::ClearResourceTimings()
{
  MOZ_ASSERT(NS_IsMainThread());
  mResourceEntries.Clear();
}

void
PerformanceBase::Mark(const nsAString& aName, ErrorResult& aRv)
{
  
  if (mUserEntries.Length() >= mResourceTimingBufferSize) {
    return;
  }

  if (IsPerformanceTimingAttribute(aName)) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  nsRefPtr<PerformanceMark> performanceMark =
    new PerformanceMark(GetAsISupports(), aName, Now());
  InsertUserEntry(performanceMark);
}

void
PerformanceBase::ClearMarks(const Optional<nsAString>& aName)
{
  ClearUserEntries(aName, NS_LITERAL_STRING("mark"));
}

DOMHighResTimeStamp
PerformanceBase::ResolveTimestampFromName(const nsAString& aName,
                                          ErrorResult& aRv)
{
  nsAutoTArray<nsRefPtr<PerformanceEntry>, 1> arr;
  DOMHighResTimeStamp ts;
  Optional<nsAString> typeParam;
  nsAutoString str;
  str.AssignLiteral("mark");
  typeParam = &str;
  GetEntriesByName(aName, typeParam, arr);
  if (!arr.IsEmpty()) {
    return arr.LastElement()->StartTime();
  }

  if (!IsPerformanceTimingAttribute(aName)) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return 0;
  }

  ts = GetPerformanceTimingFromString(aName);
  if (!ts) {
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return 0;
  }

  return DeltaFromNavigationStart(ts);
}

void
PerformanceBase::Measure(const nsAString& aName,
                         const Optional<nsAString>& aStartMark,
                         const Optional<nsAString>& aEndMark,
                         ErrorResult& aRv)
{
  
  
  if (mUserEntries.Length() >= mResourceTimingBufferSize) {
    return;
  }

  DOMHighResTimeStamp startTime;
  DOMHighResTimeStamp endTime;

  if (IsPerformanceTimingAttribute(aName)) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  if (aStartMark.WasPassed()) {
    startTime = ResolveTimestampFromName(aStartMark.Value(), aRv);
    if (NS_WARN_IF(aRv.Failed())) {
      return;
    }
  } else {
    
    
    
    startTime = 0;
  }

  if (aEndMark.WasPassed()) {
    endTime = ResolveTimestampFromName(aEndMark.Value(), aRv);
    if (NS_WARN_IF(aRv.Failed())) {
      return;
    }
  } else {
    endTime = Now();
  }

  nsRefPtr<PerformanceMeasure> performanceMeasure =
    new PerformanceMeasure(GetAsISupports(), aName, startTime, endTime);
  InsertUserEntry(performanceMeasure);
}

void
PerformanceBase::ClearMeasures(const Optional<nsAString>& aName)
{
  ClearUserEntries(aName, NS_LITERAL_STRING("measure"));
}

void
PerformanceBase::InsertUserEntry(PerformanceEntry* aEntry)
{
  mUserEntries.InsertElementSorted(aEntry,
                                   PerformanceEntryComparator());
}

void
PerformanceBase::SetResourceTimingBufferSize(uint64_t aMaxSize)
{
  mResourceTimingBufferSize = aMaxSize;
}

void
PerformanceBase::InsertResourceEntry(PerformanceEntry* aEntry)
{
  MOZ_ASSERT(aEntry);
  MOZ_ASSERT(mResourceEntries.Length() < mResourceTimingBufferSize);
  if (mResourceEntries.Length() >= mResourceTimingBufferSize) {
    return;
  }

  mResourceEntries.InsertElementSorted(aEntry,
                                       PerformanceEntryComparator());
  if (mResourceEntries.Length() == mResourceTimingBufferSize) {
    
    DispatchBufferFullEvent();
  }
}
