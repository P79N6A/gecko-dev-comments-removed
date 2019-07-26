




#include "gfxFontInfoLoader.h"
#include "nsCRT.h"
#include "nsIObserverService.h"

using namespace mozilla;
using mozilla::services::GetObserverService;

NS_IMPL_ISUPPORTS1(gfxFontInfoLoader::ShutdownObserver, nsIObserver)

NS_IMETHODIMP
gfxFontInfoLoader::ShutdownObserver::Observe(nsISupports *aSubject,
                                             const char *aTopic,
                                             const char16_t *someData)
{
    if (!nsCRT::strcmp(aTopic, "quit-application")) {
        mLoader->CancelLoader();
    } else {
        NS_NOTREACHED("unexpected notification topic");
    }
    return NS_OK;
}

void
gfxFontInfoLoader::StartLoader(uint32_t aDelay, uint32_t aInterval)
{
    mInterval = aInterval;

    
    if (mState != stateInitial && mState != stateTimerOff) {
        CancelLoader();
    }

    
    if (!mTimer) {
        mTimer = do_CreateInstance("@mozilla.org/timer;1");
        if (!mTimer) {
            NS_WARNING("Failure to create font info loader timer");
            return;
        }
    }

    
    uint32_t timerInterval;

    if (aDelay) {
        mState = stateTimerOnDelay;
        timerInterval = aDelay;
    } else {
        mState = stateTimerOnInterval;
        timerInterval = mInterval;
    }

    InitLoader();

    
    mTimer->InitWithFuncCallback(LoaderTimerCallback, this, timerInterval,
                                 nsITimer::TYPE_REPEATING_SLACK);

    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    if (obs) {
        mObserver = new ShutdownObserver(this);
        obs->AddObserver(mObserver, "quit-application", false);
    }
}

void
gfxFontInfoLoader::CancelLoader()
{
    if (mState == stateInitial) {
        return;
    }
    mState = stateTimerOff;
    if (mTimer) {
        mTimer->Cancel();
        mTimer = nullptr;
    }
    RemoveShutdownObserver();
    FinishLoader();
}

void
gfxFontInfoLoader::LoaderTimerFire()
{
    if (mState == stateTimerOnDelay) {
        mState = stateTimerOnInterval;
        mTimer->SetDelay(mInterval);
    }

    bool done = RunLoader();
    if (done) {
        CancelLoader();
    }
}

gfxFontInfoLoader::~gfxFontInfoLoader()
{
    RemoveShutdownObserver();
}

void
gfxFontInfoLoader::RemoveShutdownObserver()
{
    if (mObserver) {
        nsCOMPtr<nsIObserverService> obs = GetObserverService();
        if (obs) {
            obs->RemoveObserver(mObserver, "quit-application");
            mObserver = nullptr;
        }
    }
}
