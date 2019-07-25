





































#include "nsAppShell.h"
#include "nsWindow.h"
#include "nsThreadUtils.h"
#include "nsICommandLineRunner.h"
#include "nsIObserverService.h"
#include "nsIAppStartup.h"
#include "nsIGeolocationProvider.h"

#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "mozilla/Preferences.h"
#include "prenv.h"

#include "AndroidBridge.h"
#include "nsDeviceMotionSystem.h"
#include <android/log.h>
#include <pthread.h>
#include <wchar.h>

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

nsDeviceMotionSystem *gDeviceMotionSystem = nsnull;
nsIGeolocationUpdate *gLocationCallback = nsnull;

nsAppShell *nsAppShell::gAppShell = nsnull;

NS_IMPL_ISUPPORTS_INHERITED1(nsAppShell, nsBaseAppShell, nsIObserver)

nsAppShell::nsAppShell()
    : mQueueLock("nsAppShell.mQueueLock"),
      mCondLock("nsAppShell.mCondLock"),
      mQueueCond(mCondLock, "nsAppShell.mQueueCond"),
      mNumDraws(0)
{
    gAppShell = this;
}

nsAppShell::~nsAppShell()
{
    gAppShell = nsnull;
}

void
nsAppShell::NotifyNativeEvent()
{
    MutexAutoLock lock(mCondLock);
    mQueueCond.Notify();
}

#define PREFNAME_MATCH_OS  "intl.locale.matchOS"
#define PREFNAME_UA_LOCALE "general.useragent.locale"
static const char* kObservedPrefs[] = {
  PREFNAME_MATCH_OS,
  PREFNAME_UA_LOCALE,
  nsnull
};

nsresult
nsAppShell::Init()
{
#ifdef PR_LOGGING
    if (!gWidgetLog)
        gWidgetLog = PR_NewLogModule("Widget");
#endif

    mObserversHash.Init();

    nsresult rv = nsBaseAppShell::Init();
    AndroidBridge* bridge = AndroidBridge::Bridge();
    if (bridge)
        bridge->NotifyAppShellReady();

    nsCOMPtr<nsIObserverService> obsServ =
        mozilla::services::GetObserverService();
    if (obsServ) {
        obsServ->AddObserver(this, "xpcom-shutdown", PR_FALSE);
    }

    if (!bridge)
        return rv;

    Preferences::AddStrongObservers(this, kObservedPrefs);

    PRBool match;
    rv = Preferences::GetBool(PREFNAME_MATCH_OS, &match);
    NS_ENSURE_SUCCESS(rv, rv);

    if (match) {
        bridge->SetSelectedLocale(EmptyString());
        return NS_OK;
    }

    nsAutoString locale;
    rv = Preferences::GetLocalizedString(PREFNAME_UA_LOCALE, &locale);
    if (NS_FAILED(rv)) {
        rv = Preferences::GetString(PREFNAME_UA_LOCALE, &locale);
    }

    bridge->SetSelectedLocale(locale);
    return rv;
}

NS_IMETHODIMP
nsAppShell::Observe(nsISupports* aSubject,
                    const char* aTopic,
                    const PRUnichar* aData)
{
    if (!strcmp(aTopic, "xpcom-shutdown")) {
        
        
        mObserversHash.Clear();
        return nsBaseAppShell::Observe(aSubject, aTopic, aData);
    } else if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) && aData && (
                   nsDependentString(aData).Equals(
                       NS_LITERAL_STRING(PREFNAME_UA_LOCALE)) ||
                   nsDependentString(aData).Equals(
                       NS_LITERAL_STRING(PREFNAME_MATCH_OS)))) {
        AndroidBridge* bridge = AndroidBridge::Bridge();
        if (!bridge) {
            return NS_OK;
        }

        PRBool match;
        nsresult rv = Preferences::GetBool(PREFNAME_MATCH_OS, &match);
        NS_ENSURE_SUCCESS(rv, rv);

        if (match) {
            bridge->SetSelectedLocale(EmptyString());
            return NS_OK;
        }

        nsAutoString locale;
        if (NS_FAILED(Preferences::GetLocalizedString(PREFNAME_UA_LOCALE,
                                                      &locale))) {
            locale = Preferences::GetString(PREFNAME_UA_LOCALE);
        }

        bridge->SetSelectedLocale(locale);
        return NS_OK;
    }
    return NS_OK;
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

    nsAutoPtr<AndroidGeckoEvent> curEvent;
    AndroidGeckoEvent *nextEvent;
    {
        MutexAutoLock lock(mCondLock);

        curEvent = GetNextEvent();
        if (!curEvent && mayWait) {
            
#if defined(ANDROID_DEBUG_EVENTS)
            PRTime t0, t1;
            EVLOG("nsAppShell: waiting on mQueueCond");
            t0 = PR_Now();

            mQueueCond.Wait(PR_MillisecondsToInterval(10000));
            t1 = PR_Now();
            EVLOG("nsAppShell: wait done, waited %d ms", (int)(t1-t0)/1000);
#else
            mQueueCond.Wait();
#endif

            curEvent = GetNextEvent();
        }
    }

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

    switch (curEvent->Type()) {
    case AndroidGeckoEvent::NATIVE_POKE:
        NativeEventCallback();
        break;

    case AndroidGeckoEvent::ACCELERATION_EVENT:
        gDeviceMotionSystem->DeviceMotionChanged(nsIDeviceMotionData::TYPE_ACCELERATION,
                                                 -curEvent->X(),
                                                 curEvent->Y(),
                                                 curEvent->Z());
        break;

    case AndroidGeckoEvent::ORIENTATION_EVENT:
        gDeviceMotionSystem->DeviceMotionChanged(nsIDeviceMotionData::TYPE_ORIENTATION,
                                                 -curEvent->Alpha(),
                                                 curEvent->Beta(),
                                                 curEvent->Gamma());
        break;

    case AndroidGeckoEvent::LOCATION_EVENT: {
        if (!gLocationCallback)
            break;

        nsGeoPosition* p = curEvent->GeoPosition();
        nsGeoPositionAddress* a = curEvent->GeoAddress();

        if (p) {
            p->SetAddress(a);
            gLocationCallback->Update(curEvent->GeoPosition());
        }
        else
            NS_WARNING("Received location event without geoposition!");
        break;
    }

    case AndroidGeckoEvent::ACTIVITY_STOPPING: {
        nsCOMPtr<nsIObserverService> obsServ =
            mozilla::services::GetObserverService();
        NS_NAMED_LITERAL_STRING(minimize, "heap-minimize");
        obsServ->NotifyObservers(nsnull, "memory-pressure", minimize.get());

        break;
    }

    case AndroidGeckoEvent::ACTIVITY_SHUTDOWN: {
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
        
        
        
        nsIPrefService* prefs = Preferences::GetService();
        if (prefs) {
            prefs->SavePrefFile(nsnull);
        }

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

        const char *argv[3] = {
            "dummyappname",
            "-remote",
            uri
        };
        nsresult rv = cmdline->Init(3, const_cast<char **>(argv), nsnull, nsICommandLine::STATE_REMOTE_AUTO);
        if (NS_SUCCEEDED(rv))
            cmdline->Run();
        nsMemory::Free(uri);
        break;
    }

    default:
        nsWindow::OnGlobalAndroidEvent(curEvent);
    }

    EVLOG("nsAppShell: -- done event %p %d", (void*)curEvent.get(), curEvent->Type());

    return true;
}

AndroidGeckoEvent*
nsAppShell::GetNextEvent()
{
    AndroidGeckoEvent *ae = nsnull;
    MutexAutoLock lock(mQueueLock);
    if (mEventQueue.Length()) {
        ae = mEventQueue[0];
        mEventQueue.RemoveElementAt(0);
        if (ae->Type() == AndroidGeckoEvent::DRAW) {
            mNumDraws--;
        }
    }

    return ae;
}

AndroidGeckoEvent*
nsAppShell::PeekNextEvent()
{
    AndroidGeckoEvent *ae = nsnull;
    MutexAutoLock lock(mQueueLock);
    if (mEventQueue.Length()) {
        ae = mEventQueue[0];
    }

    return ae;
}

void
nsAppShell::PostEvent(AndroidGeckoEvent *ae)
{
    {
        MutexAutoLock lock(mQueueLock);
        mEventQueue.AppendElement(ae);
        if (ae->Type() == AndroidGeckoEvent::DRAW) {
            mNumDraws++;
        }
    }
    NotifyNativeEvent();
}

void
nsAppShell::RemoveNextEvent()
{
    AndroidGeckoEvent *ae = nsnull;
    MutexAutoLock lock(mQueueLock);
    if (mEventQueue.Length()) {
        ae = mEventQueue[0];
        mEventQueue.RemoveElementAt(0);
        if (ae->Type() == AndroidGeckoEvent::DRAW) {
            mNumDraws--;
        }
    }
}

void
nsAppShell::OnResume()
{
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
        unused << rv;
    }
}

void
nsAppShell::RemoveObserver(const nsAString &aObserverKey)
{
    mObserversHash.Remove(aObserverKey);
}



class NotifyObserversCaller : public nsRunnable {
public:
    NotifyObserversCaller(nsISupports *aSupports,
                          const char *aTopic, const PRUnichar *aData) :
        mSupports(aSupports), mTopic(aTopic), mData(aData) {
    }

    NS_IMETHOD Run() {
        nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
        if (os)
            os->NotifyObservers(mSupports, mTopic.get(), mData.get());

        return NS_OK;
    }

private:
    nsCOMPtr<nsISupports> mSupports;
    nsCString mTopic;
    nsString mData;
};

void
nsAppShell::NotifyObservers(nsISupports *aSupports,
                            const char *aTopic,
                            const PRUnichar *aData)
{
    
    nsCOMPtr<nsIRunnable> caller =
        new NotifyObserversCaller(aSupports, aTopic, aData);
    NS_DispatchToMainThread(caller);
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
