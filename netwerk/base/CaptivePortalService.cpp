



#include "CaptivePortalService.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "nsIObserverService.h"
#include "nsXULAppAPI.h"

#define kInterfaceName "captive-portal-inteface"

static const char kOpenCaptivePortalLoginEvent[] = "captive-portal-login";
static const char kAbortCaptivePortalLoginEvent[] = "captive-portal-login-abort";
static const char kCaptivePortalLoginSuccessEvent[] = "captive-portal-login-success";

static const uint32_t kDefaultInterval = 60*1000; 

static PRLogModuleInfo *gCaptivePortalLog = nullptr;
#undef LOG
#define LOG(args) MOZ_LOG(gCaptivePortalLog, mozilla::LogLevel::Debug, args)

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS(CaptivePortalService, nsICaptivePortalService, nsIObserver,
                  nsISupportsWeakReference, nsITimerCallback,
                  nsICaptivePortalCallback)

CaptivePortalService::CaptivePortalService()
  : mState(UNKNOWN)
  , mStarted(false)
  , mInitialized(false)
  , mRequestInProgress(false)
  , mEverBeenCaptive(false)
  , mDelay(kDefaultInterval)
  , mSlackCount(0)
  , mMinInterval(kDefaultInterval)
  , mMaxInterval(25*kDefaultInterval)
  , mBackoffFactor(5.0)
{
  mLastChecked = TimeStamp::Now();
}

CaptivePortalService::~CaptivePortalService()
{
}

nsresult
CaptivePortalService::PerformCheck()
{
  LOG(("CaptivePortalService::PerformCheck mRequestInProgress:%d mInitialized:%d mStarted:%d\n",
        mRequestInProgress, mInitialized, mStarted));
  
  if (mRequestInProgress || !mInitialized || !mStarted) {
    return NS_OK;
  }

  nsresult rv;
  if (!mCaptivePortalDetector) {
    mCaptivePortalDetector =
      do_GetService("@mozilla.org/toolkit/captive-detector;1", &rv);
    if (NS_FAILED(rv)) {
        LOG(("Unable to get a captive portal detector\n"));
        return rv;
    }
  }

  LOG(("CaptivePortalService::PerformCheck - Calling CheckCaptivePortal\n"));
  mRequestInProgress = true;
  mCaptivePortalDetector->CheckCaptivePortal(
    NS_LITERAL_STRING(kInterfaceName).get(), this);
  return NS_OK;
}

nsresult
CaptivePortalService::RearmTimer()
{
  
  if (mTimer) {
    mTimer->Cancel();
  }

  if (!mTimer) {
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  }

  if (mTimer && mDelay > 0) {
    LOG(("CaptivePortalService - Reloading timer with delay %u\n", mDelay));
    return mTimer->InitWithCallback(this, mDelay, nsITimer::TYPE_ONE_SHOT);
  }

  return NS_OK;
}

nsresult
CaptivePortalService::Initialize()
{
  if (mInitialized || XRE_GetProcessType() != GeckoProcessType_Default) {
    return NS_OK;
  }
  mInitialized = true;

  if (!gCaptivePortalLog) {
    gCaptivePortalLog = PR_NewLogModule("CaptivePortalService");
  }

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->AddObserver(this, kOpenCaptivePortalLoginEvent, true);
    observerService->AddObserver(this, kAbortCaptivePortalLoginEvent, true);
    observerService->AddObserver(this, kCaptivePortalLoginSuccessEvent, true);
  }

  LOG(("Initialized CaptivePortalService\n"));
  return NS_OK;
}

nsresult
CaptivePortalService::Start()
{
  if (!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (mStarted) {
    return NS_OK;
  }

  mStarted = true;
  mEverBeenCaptive = false;

  
  Preferences::GetUint("network.captive-portal-service.minInterval", &mMinInterval);
  Preferences::GetUint("network.captive-portal-service.maxInterval", &mMaxInterval);
  Preferences::GetFloat("network.captive-portal-service.backoffFactor", &mBackoffFactor);

  LOG(("CaptivePortalService::Start min:%u max:%u backoff:%.2f\n",
       mMinInterval, mMaxInterval, mBackoffFactor));

  mSlackCount = 0;
  mDelay = mMinInterval;

  
  PerformCheck();
  RearmTimer();
  return NS_OK;
}

nsresult
CaptivePortalService::Stop()
{
  LOG(("CaptivePortalService::Stop\n"));

  if (!mStarted) {
    return NS_OK;
  }

  if (mTimer) {
    mTimer->Cancel();
  }
  mTimer = nullptr;
  mRequestInProgress = false;
  mStarted = false;
  if (mCaptivePortalDetector) {
    mCaptivePortalDetector->Abort(NS_LITERAL_STRING(kInterfaceName).get());
  }
  mCaptivePortalDetector = nullptr;
  return NS_OK;
}





NS_IMETHODIMP
CaptivePortalService::GetState(int32_t *aState)
{
  *aState = UNKNOWN;
  if (!mInitialized) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  *aState = mState;
  return NS_OK;
}

NS_IMETHODIMP
CaptivePortalService::RecheckCaptivePortal()
{
  LOG(("CaptivePortalService::RecheckCaptivePortal\n"));

  
  
  mSlackCount = 0;
  mDelay = mMinInterval;

  PerformCheck();
  RearmTimer();
  return NS_OK;
}

NS_IMETHODIMP
CaptivePortalService::GetLastChecked(uint64_t *aLastChecked)
{
  double duration = (TimeStamp::Now() - mLastChecked).ToMilliseconds();
  *aLastChecked = static_cast<uint64_t>(duration);
  return NS_OK;
}






NS_IMETHODIMP
CaptivePortalService::Notify(nsITimer *aTimer)
{
  LOG(("CaptivePortalService::Notify\n"));
  MOZ_ASSERT(aTimer == mTimer);

  PerformCheck();

  
  
  
  mSlackCount++;
  if (mSlackCount % 10 == 0) {
    mDelay = mDelay * mBackoffFactor;
  }
  if (mDelay > mMaxInterval) {
    mDelay = mMaxInterval;
  }

  
  RearmTimer();

  return NS_OK;
}




NS_IMETHODIMP
CaptivePortalService::Observe(nsISupports *aSubject,
                              const char * aTopic,
                              const char16_t * aData)
{
  LOG(("CaptivePortalService::Observe() topic=%s\n", aTopic));
  if (!strcmp(aTopic, kOpenCaptivePortalLoginEvent)) {
    
    
    mState = LOCKED_PORTAL;
    mLastChecked = TimeStamp::Now();
    mEverBeenCaptive = true;
  } else if (!strcmp(aTopic, kCaptivePortalLoginSuccessEvent)) {
    
    mState = UNLOCKED_PORTAL;
    mLastChecked = TimeStamp::Now();
    mRequestInProgress = false;
    mSlackCount = 0;
    mDelay = mMinInterval;
    RearmTimer();
  } else if (!strcmp(aTopic, kAbortCaptivePortalLoginEvent)) {
    
    mRequestInProgress = false;
    mState = UNKNOWN;
    mLastChecked = TimeStamp::Now();
    mSlackCount = 0;
  }
  return NS_OK;
}




NS_IMETHODIMP
CaptivePortalService::Prepare()
{
  LOG(("CaptivePortalService::Prepare\n"));
  
  if (mCaptivePortalDetector) {
    mCaptivePortalDetector->FinishPreparation(NS_LITERAL_STRING(kInterfaceName).get());
  }
  return NS_OK;
}

NS_IMETHODIMP
CaptivePortalService::Complete(bool success)
{
  LOG(("CaptivePortalService::Complete(success=%d) mState=%d\n", success, mState));
  mLastChecked = TimeStamp::Now();
  if ((mState == UNKNOWN || mState == NOT_CAPTIVE) && success) {
    mState = NOT_CAPTIVE;
    
    
    if (!mEverBeenCaptive) {
      mDelay = 0;
      if (mTimer) {
        mTimer->Cancel();
      }
    }
  }

  mRequestInProgress = false;
  return NS_OK;
}

} 
} 
