





































#include "nsAppShell.h"
#include "nsWindow.h"
#include "nsThreadUtils.h"
#include "nsICommandLineRunner.h"
#include "nsIObserverService.h"
#include "nsIAppStartup.h"
#include "nsIGeolocationProvider.h"
#include "nsIPrefService.h"

#include "mozilla/Services.h"
#include "prenv.h"

#include "AndroidBridge.h"
#include "nsAccelerometerSystem.h"
#include <android/log.h>
#include <pthread.h>

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#include "prlog.h"
#endif

#ifdef ANDROID_DEBUG_EVENTS
#define EVLOG(args...)  ALOG(args)
#else
#define EVLOG(args...) do { } while (0)
#endif

using namespace mozilla;

#ifdef PR_LOGGING
PRLogModuleInfo *gWidgetLog = nsnull;
#endif

nsAccelerometerSystem *gAccel = nsnull;
nsIGeolocationUpdate *gLocationCallback = nsnull;

nsAppShell *nsAppShell::gAppShell = nsnull;
AndroidGeckoEvent *nsAppShell::gEarlyEvent = nsnull;

nsAppShell::nsAppShell()
    : mQueueLock(nsnull),
      mCondLock(nsnull),
      mQueueCond(nsnull),
      mPausedLock(nsnull),
      mPaused(nsnull),
      mNumDraws(0)
{
    gAppShell = this;
    if (gEarlyEvent) {
        mEventQueue.AppendElement(gEarlyEvent);
        gEarlyEvent = nsnull;
    }
}

nsAppShell::~nsAppShell()
{
    gAppShell = nsnull;
}

void
nsAppShell::NotifyNativeEvent()
{
    PR_Lock(mCondLock);
    PR_NotifyCondVar(mQueueCond);
    PR_Unlock(mCondLock);
}

nsresult
nsAppShell::Init()
{
#ifdef PR_LOGGING
    if (!gWidgetLog)
        gWidgetLog = PR_NewLogModule("Widget");
#endif

    mQueueLock = PR_NewLock();
    mCondLock = PR_NewLock();
    mPausedLock = PR_NewLock();
    mQueueCond = PR_NewCondVar(mCondLock);
    mPaused = PR_NewCondVar(mPausedLock);

    mObserversHash.Init();

    return nsBaseAppShell::Init();
}


void
nsAppShell::ScheduleNativeEventCallback()
{
    EVLOG("nsAppShell::ScheduleNativeEventCallback pth: %p thread: %p main: %d", (void*) pthread_self(), (void*) NS_GetCurrentThread(), NS_IsMainThread());

    
    PostEvent(new AndroidGeckoEvent(AndroidGeckoEvent::NATIVE_POKE));
}


PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
    EVLOG("nsAppShell::ProcessNextNativeEvent %d", mayWait);

    PR_Lock(mCondLock);

    nsAutoPtr<AndroidGeckoEvent> curEvent;
    AndroidGeckoEvent *nextEvent;

    curEvent = GetNextEvent();
    if (!curEvent && mayWait) {
        
#if defined(ANDROID_DEBUG_EVENTS)
        PRTime t0, t1;
        EVLOG("nsAppShell: waiting on mQueueCond");
        t0 = PR_Now();

        PR_WaitCondVar(mQueueCond, PR_MillisecondsToInterval(10000));
        t1 = PR_Now();
        EVLOG("nsAppShell: wait done, waited %d ms", (int)(t1-t0)/1000);
#else
        PR_WaitCondVar(mQueueCond, PR_INTERVAL_NO_TIMEOUT);
#endif

        curEvent = GetNextEvent();
    }

    PR_Unlock(mCondLock);

    if (!curEvent)
        return false;

    

    nextEvent = PeekNextEvent();

    while (nextEvent) {
        int curType = curEvent->Type();
        int nextType = nextEvent->Type();

        while (nextType == AndroidGeckoEvent::DRAW &&
               mNumDraws > 1)
        {
            
            
            
            
            
            
            RemoveNextEvent();
            delete nextEvent;

#if defined(ANDROID_DEBUG_EVENTS)
            ALOG("# Removing DRAW event (%d outstanding)", mNumDraws);
#endif

            nextEvent = PeekNextEvent();
            nextType = nextEvent->Type();
        }

        
        
        if (nextType != curType)
            break;

        
        if (curType != AndroidGeckoEvent::MOTION_EVENT)
            break;

        if (!(curEvent->Action() == AndroidMotionEvent::ACTION_MOVE &&
              nextEvent->Action() == AndroidMotionEvent::ACTION_MOVE))
            break;

#if defined(ANDROID_DEBUG_EVENTS)
        ALOG("# Removing % 2d event", curType);
#endif

        RemoveNextEvent();
        curEvent = nextEvent;
        nextEvent = PeekNextEvent();
    }

    EVLOG("nsAppShell: event %p %d [ndraws %d]", (void*)curEvent.get(), curEvent->Type(), mNumDraws);

    nsWindow *target = (nsWindow*) curEvent->NativeWindow();

    switch (curEvent->Type()) {
    case AndroidGeckoEvent::NATIVE_POKE:
        NativeEventCallback();
        break;

    case AndroidGeckoEvent::SENSOR_EVENT:
        gAccel->AccelerationChanged(-curEvent->X(), curEvent->Y(), curEvent->Z());
        break;

    case AndroidGeckoEvent::LOCATION_EVENT:
        if (!gLocationCallback)
            break;

        if (curEvent->GeoPosition())
            gLocationCallback->Update(curEvent->GeoPosition());
        else
            NS_WARNING("Received location event without geoposition!");
        break;

    case AndroidGeckoEvent::ACTIVITY_STOPPING: {
        nsCOMPtr<nsIObserverService> obsServ =
          mozilla::services::GetObserverService();
        NS_NAMED_LITERAL_STRING(context, "shutdown-persist");
        obsServ->NotifyObservers(nsnull, "quit-application-granted", nsnull);
        obsServ->NotifyObservers(nsnull, "quit-application-forced", nsnull);
        obsServ->NotifyObservers(nsnull, "profile-change-net-teardown", context.get());
        obsServ->NotifyObservers(nsnull, "profile-change-teardown", context.get());
        obsServ->NotifyObservers(nsnull, "profile-before-change", context.get());
        nsCOMPtr<nsIAppStartup> appSvc = do_GetService("@mozilla.org/toolkit/app-startup;1");
        if (appSvc)
            appSvc->Quit(nsIAppStartup::eForceQuit);
        break;
    }

    case AndroidGeckoEvent::ACTIVITY_PAUSING: {
        
        
        
        nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs)
            prefs->SavePrefFile(nsnull);

        
        
        PR_WaitCondVar(mPaused, PR_INTERVAL_NO_TIMEOUT);
        break;
    }

    case AndroidGeckoEvent::LOAD_URI: {
        nsCOMPtr<nsICommandLineRunner> cmdline
            (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
        if (!cmdline)
            break;

        char *uri = ToNewUTF8String(curEvent->Characters());
        if (!uri)
            break;

        char* argv[3] = {
            "dummyappname",
            "-remote",
            uri
        };
        nsresult rv = cmdline->Init(3, argv, nsnull, nsICommandLine::STATE_REMOTE_AUTO);
        if (NS_SUCCEEDED(rv))
            cmdline->Run();
        nsMemory::Free(uri);
        break;
    }

    default:
        if (target)
            target->OnAndroidEvent(curEvent);
        else
            nsWindow::OnGlobalAndroidEvent(curEvent);
    }

    EVLOG("nsAppShell: -- done event %p %d", (void*)curEvent.get(), curEvent->Type());

    return true;
}

AndroidGeckoEvent*
nsAppShell::GetNextEvent()
{
    AndroidGeckoEvent *ae = nsnull;
    PR_Lock(mQueueLock);
    if (mEventQueue.Length()) {
        ae = mEventQueue[0];
        mEventQueue.RemoveElementAt(0);
        if (ae->Type() == AndroidGeckoEvent::DRAW) {
            mNumDraws--;
        }
    }
    PR_Unlock(mQueueLock);

    return ae;
}

AndroidGeckoEvent*
nsAppShell::PeekNextEvent()
{
    AndroidGeckoEvent *ae = nsnull;
    PR_Lock(mQueueLock);
    if (mEventQueue.Length()) {
        ae = mEventQueue[0];
    }
    PR_Unlock(mQueueLock);

    return ae;
}

void
nsAppShell::PostEvent(AndroidGeckoEvent *ae)
{
    PR_Lock(mQueueLock);
    mEventQueue.AppendElement(ae);
    if (ae->Type() == AndroidGeckoEvent::DRAW) {
        mNumDraws++;
    }
    PR_Unlock(mQueueLock);
    NotifyNativeEvent();
}

void
nsAppShell::RemoveNextEvent()
{
    AndroidGeckoEvent *ae = nsnull;
    PR_Lock(mQueueLock);
    if (mEventQueue.Length()) {
        ae = mEventQueue[0];
        mEventQueue.RemoveElementAt(0);
        if (ae->Type() == AndroidGeckoEvent::DRAW) {
            mNumDraws--;
        }
    }
    PR_Unlock(mQueueLock);
}

void
nsAppShell::OnResume()
{
    PR_Lock(mPausedLock);
    PR_NotifyCondVar(mPaused);
    PR_Unlock(mPausedLock);

}

nsresult
nsAppShell::AddObserver(const nsAString &aObserverKey, nsIObserver *aObserver)
{
    NS_ASSERTION(aObserver != nsnull, "nsAppShell::AddObserver: aObserver is null!");
    return mObserversHash.Put(aObserverKey, aObserver) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}




class ObserverCaller : public nsRunnable {
public:
    ObserverCaller(nsIObserver *aObserver, const char *aTopic, const PRUnichar *aData) :
        mObserver(aObserver), mTopic(aTopic), mData(aData) {
        NS_ASSERTION(aObserver != nsnull, "ObserverCaller: aObserver is null!");
    }

    NS_IMETHOD Run() {
        ALOG("ObserverCaller::Run: observer = %p, topic = '%s')",
             (nsIObserver*)mObserver, mTopic.get());
        mObserver->Observe(nsnull, mTopic.get(), mData.get());
        return NS_OK;
    }

private:
    nsCOMPtr<nsIObserver> mObserver;
    nsCString mTopic;
    nsString mData;
};

void
nsAppShell::CallObserver(const nsAString &aObserverKey, const nsAString &aTopic, const nsAString &aData)
{
    nsCOMPtr<nsIObserver> observer;
    mObserversHash.Get(aObserverKey, getter_AddRefs(observer));

    if (!observer) {
        ALOG("nsAppShell::CallObserver: Observer was not found!");
        return;
    }

    const NS_ConvertUTF16toUTF8 sTopic(aTopic);
    const nsPromiseFlatString& sData = PromiseFlatString(aData);
    
    if (NS_IsMainThread()) {
        
        observer->Observe(nsnull, sTopic.get(), sData.get());
    } else {
        
        nsCOMPtr<nsIRunnable> observerCaller = new ObserverCaller(observer, sTopic.get(), sData.get());
        nsresult rv = NS_DispatchToMainThread(observerCaller);
        ALOG("NS_DispatchToMainThread result: %d", rv);
    }
}

void
nsAppShell::RemoveObserver(const nsAString &aObserverKey)
{
    mObserversHash.Remove(aObserverKey);
}


namespace mozilla {

bool ProcessNextEvent()
{
    return nsAppShell::gAppShell->ProcessNextNativeEvent(PR_TRUE) ? true : false;
}

void NotifyEvent()
{
    nsAppShell::gAppShell->NotifyNativeEvent();
}

}
