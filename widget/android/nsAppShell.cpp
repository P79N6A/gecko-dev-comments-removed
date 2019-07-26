






































#include "base/basictypes.h"
#include "nspr/prtypes.h"

#include "mozilla/Hal.h"
#include "nsAppShell.h"
#include "nsWindow.h"
#include "nsThreadUtils.h"
#include "nsICommandLineRunner.h"
#include "nsIObserverService.h"
#include "nsIAppStartup.h"
#include "nsIGeolocationProvider.h"
#include "nsCacheService.h"
#include "nsIDOMEventListener.h"
#include "nsDOMNotifyPaintEvent.h"
#include "nsIDOMClientRectList.h"
#include "nsIDOMClientRect.h"

#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "mozilla/Preferences.h"
#include "mozilla/Hal.h"
#include "prenv.h"

#include "AndroidBridge.h"
#include <android/log.h>
#include <pthread.h>
#include <wchar.h>

#include "mozilla/dom/ScreenOrientation.h"

#include "sampler.h"
#ifdef MOZ_ANDROID_HISTORY
#include "nsAndroidHistory.h"
#endif

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#include "prlog.h"
#endif

#ifdef DEBUG_ANDROID_EVENTS
#define EVLOG(args...)  ALOG(args)
#else
#define EVLOG(args...) do { } while (0)
#endif

using namespace mozilla;

#ifdef PR_LOGGING
PRLogModuleInfo *gWidgetLog = nsnull;
#endif

nsIGeolocationUpdate *gLocationCallback = nsnull;
nsAutoPtr<mozilla::AndroidGeckoEvent> gLastSizeChange;

nsAppShell *nsAppShell::gAppShell = nsnull;

NS_IMPL_ISUPPORTS_INHERITED1(nsAppShell, nsBaseAppShell, nsIObserver)

class AfterPaintListener : public nsIDOMEventListener {
  public:
    NS_DECL_ISUPPORTS

    void Register(nsIDOMWindow* window) {
        if (mEventTarget)
            Unregister();
        nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(window);
        if (!win)
            return;
        mEventTarget = win->GetChromeEventHandler();
        if (mEventTarget)
            mEventTarget->AddEventListener(NS_LITERAL_STRING("MozAfterPaint"), this, false);
    }

    void Unregister() {
        if (mEventTarget)
            mEventTarget->RemoveEventListener(NS_LITERAL_STRING("MozAfterPaint"), this, false);
        mEventTarget = nsnull;
    }

    virtual nsresult HandleEvent(nsIDOMEvent* aEvent) {
        nsCOMPtr<nsIDOMNotifyPaintEvent> paintEvent = do_QueryInterface(aEvent);
        if (!paintEvent)
            return NS_OK;

        nsCOMPtr<nsIDOMClientRectList> rects;
        paintEvent->GetClientRects(getter_AddRefs(rects));
        if (!rects)
            return NS_OK;
        PRUint32 length;
        rects->GetLength(&length);
        for (PRUint32 i = 0; i < length; ++i) {
            float top, left, bottom, right;
            nsCOMPtr<nsIDOMClientRect> rect = rects->GetItemAt(i);
            if (!rect)
                continue;
            rect->GetTop(&top);
            rect->GetLeft(&left);
            rect->GetRight(&right);
            rect->GetBottom(&bottom);
            AndroidBridge::NotifyPaintedRect(top, left, bottom, right);
        }
        return NS_OK;
    }

    ~AfterPaintListener() {
        if (mEventTarget)
            Unregister();
    }

  private:
    nsCOMPtr<nsIDOMEventTarget> mEventTarget;
};

NS_IMPL_ISUPPORTS1(AfterPaintListener, nsIDOMEventListener)
nsCOMPtr<AfterPaintListener> sAfterPaintListener = nsnull;

nsAppShell::nsAppShell()
    : mQueueLock("nsAppShell.mQueueLock"),
      mCondLock("nsAppShell.mCondLock"),
      mQueueCond(mCondLock, "nsAppShell.mQueueCond"),
      mQueuedDrawEvent(nsnull),
      mQueuedViewportEvent(nsnull),
      mAllowCoalescingNextDraw(false)
{
    gAppShell = this;
    sAfterPaintListener = new AfterPaintListener();
}

nsAppShell::~nsAppShell()
{
    gAppShell = nsnull;
    delete sAfterPaintListener;
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

    nsCOMPtr<nsIObserverService> obsServ =
        mozilla::services::GetObserverService();
    if (obsServ) {
        obsServ->AddObserver(this, "xpcom-shutdown", false);
    }

    if (!bridge)
        return rv;

    Preferences::AddStrongObservers(this, kObservedPrefs);

    bool match;
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

        bool match;
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

bool
nsAppShell::ProcessNextNativeEvent(bool mayWait)
{
    EVLOG("nsAppShell::ProcessNextNativeEvent %d", mayWait);

    SAMPLE_LABEL("nsAppShell", "ProcessNextNativeEvent");
    nsAutoPtr<AndroidGeckoEvent> curEvent;
    {
        MutexAutoLock lock(mCondLock);

        curEvent = PopNextEvent();
        if (!curEvent && mayWait) {
            SAMPLE_LABEL("nsAppShell::ProcessNextNativeEvent", "Wait");
            
#if defined(DEBUG_ANDROID_EVENTS)
            PRTime t0, t1;
            EVLOG("nsAppShell: waiting on mQueueCond");
            t0 = PR_Now();
            mQueueCond.Wait(PR_MillisecondsToInterval(10000));
            t1 = PR_Now();
            EVLOG("nsAppShell: wait done, waited %d ms", (int)(t1-t0)/1000);
#else
            mQueueCond.Wait();
#endif

            curEvent = PopNextEvent();
        }
    }

    if (!curEvent)
        return false;

    EVLOG("nsAppShell: event %p %d", (void*)curEvent.get(), curEvent->Type());

    switch (curEvent->Type()) {
    case AndroidGeckoEvent::NATIVE_POKE:
        NativeEventCallback();
        break;

    case AndroidGeckoEvent::SENSOR_EVENT:
      {
        InfallibleTArray<float> values;
        mozilla::hal::SensorType type = (mozilla::hal::SensorType) curEvent->Flags();

        switch (type) {
          case hal::SENSOR_ORIENTATION:
          case hal::SENSOR_LINEAR_ACCELERATION:
          case hal::SENSOR_ACCELERATION:
          case hal::SENSOR_GYROSCOPE:
            values.AppendElement(curEvent->X());
            values.AppendElement(curEvent->Y()); 
            values.AppendElement(curEvent->Z());
            break;

        case hal::SENSOR_LIGHT:
        case hal::SENSOR_PROXIMITY:
            values.AppendElement(curEvent->X());
            break;

        default:
            __android_log_print(ANDROID_LOG_ERROR,
                                "Gecko", "### SENSOR_EVENT fired, but type wasn't known %d",
                                type);
        }

        const hal::SensorAccuracyType &accuracy = (hal::SensorAccuracyType) curEvent->MetaState();
        hal::SensorData sdata(type, PR_Now(), values, accuracy);
        hal::NotifySensorChange(sdata);
      }
      break;

    case AndroidGeckoEvent::LOCATION_EVENT: {
        if (!gLocationCallback)
            break;

        nsGeoPosition* p = curEvent->GeoPosition();
        if (p)
            gLocationCallback->Update(curEvent->GeoPosition());
        else
            NS_WARNING("Received location event without geoposition!");
        break;
    }

    case AndroidGeckoEvent::ACTIVITY_STOPPING: {
        if (curEvent->Flags() > 0)
            break;

        nsCOMPtr<nsIObserverService> obsServ =
            mozilla::services::GetObserverService();
        NS_NAMED_LITERAL_STRING(minimize, "heap-minimize");
        obsServ->NotifyObservers(nsnull, "memory-pressure", minimize.get());
        obsServ->NotifyObservers(nsnull, "application-background", nsnull);

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
        if (curEvent->Flags() == 0) {
            
            
            nsCOMPtr<nsIObserverService> obsServ =
                mozilla::services::GetObserverService();
            obsServ->NotifyObservers(nsnull, "application-background", nsnull);

            
            
            
            if (nsCacheService::GlobalInstance())
                nsCacheService::GlobalInstance()->Shutdown();
        }

        
        
        
        nsIPrefService* prefs = Preferences::GetService();
        if (prefs) {
            
            nsCOMPtr<nsIPrefBranch> prefBranch;
            prefs->GetBranch("browser.sessionstore.", getter_AddRefs(prefBranch));
            if (prefBranch)
                prefBranch->SetIntPref("recent_crashes", 0);

            prefs->SavePrefFile(nsnull);
        }

        break;
    }

    case AndroidGeckoEvent::ACTIVITY_START: {
        if (curEvent->Flags() > 0)
            break;

        nsCOMPtr<nsIObserverService> obsServ =
            mozilla::services::GetObserverService();
        obsServ->NotifyObservers(nsnull, "application-foreground", nsnull);

        break;
    }

    case AndroidGeckoEvent::PAINT_LISTEN_START_EVENT: {
        nsCOMPtr<nsIDOMWindow> domWindow;
        nsCOMPtr<nsIBrowserTab> tab;
        mBrowserApp->GetBrowserTab(curEvent->MetaState(), getter_AddRefs(tab));
        if (!tab)
            break;

        tab->GetWindow(getter_AddRefs(domWindow));
        if (!domWindow)
            break;

        sAfterPaintListener->Register(domWindow);
        break;
    }

    case AndroidGeckoEvent::SCREENSHOT: {
        if (!mBrowserApp)
            break;

        AndroidBridge* bridge = AndroidBridge::Bridge();
        if (!bridge)
            break;

        PRInt32 token = curEvent->Flags();

        nsCOMPtr<nsIDOMWindow> domWindow;
        nsCOMPtr<nsIBrowserTab> tab;
        mBrowserApp->GetBrowserTab(curEvent->MetaState(), getter_AddRefs(tab));
        if (!tab)
            break;

        tab->GetWindow(getter_AddRefs(domWindow));
        if (!domWindow)
            break;

        float scale = 1.0;
        nsTArray<nsIntPoint> points = curEvent->Points();
        NS_ASSERTION(points.Length() == 4, "Screenshot event does not have enough coordinates");
        bridge->TakeScreenshot(domWindow, points[0].x, points[0].y, points[1].x, points[1].y, points[3].x, points[3].y, curEvent->MetaState(), scale, curEvent->Flags());
        break;
    }

    case AndroidGeckoEvent::VIEWPORT:
    case AndroidGeckoEvent::BROADCAST: {

        if (curEvent->Characters().Length() == 0)
            break;

        nsCOMPtr<nsIObserverService> obsServ =
            mozilla::services::GetObserverService();

        const NS_ConvertUTF16toUTF8 topic(curEvent->Characters());
        const nsPromiseFlatString& data = PromiseFlatString(curEvent->CharactersExtra());

        obsServ->NotifyObservers(nsnull, topic.get(), data.get());
        break;
    }

    case AndroidGeckoEvent::LOAD_URI: {
        nsCOMPtr<nsICommandLineRunner> cmdline
            (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
        if (!cmdline)
            break;

        if (curEvent->Characters().Length() == 0)
            break;

        char *uri = ToNewUTF8String(curEvent->Characters());
        if (!uri)
            break;

        char *flag = ToNewUTF8String(curEvent->CharactersExtra());

        const char *argv[4] = {
            "dummyappname",
            "-url",
            uri,
            flag ? flag : ""
        };
        nsresult rv = cmdline->Init(4, const_cast<char **>(argv), nsnull, nsICommandLine::STATE_REMOTE_AUTO);
        if (NS_SUCCEEDED(rv))
            cmdline->Run();
        nsMemory::Free(uri);
        if (flag)
            nsMemory::Free(flag);
        break;
    }

    case AndroidGeckoEvent::SIZE_CHANGED: {
        
        if (curEvent != gLastSizeChange) {
            gLastSizeChange = new AndroidGeckoEvent(curEvent);
        }
        nsWindow::OnGlobalAndroidEvent(curEvent);
        break;
    }

    case AndroidGeckoEvent::VISITED: {
#ifdef MOZ_ANDROID_HISTORY
        nsAndroidHistory::NotifyURIVisited(nsString(curEvent->Characters()));
#endif
        break;
    }

    case AndroidGeckoEvent::NETWORK_CHANGED: {
        hal::NotifyNetworkChange(hal::NetworkInformation(curEvent->Bandwidth(),
                                                         curEvent->CanBeMetered()));
        break;
    }

    case AndroidGeckoEvent::ACTIVITY_RESUMING: {
        if (curEvent->Flags() == 0) {
            
            
            
            if (nsCacheService::GlobalInstance())
                nsCacheService::GlobalInstance()->Init();

            
            
            nsCOMPtr<nsIObserverService> obsServ =
                mozilla::services::GetObserverService();
            obsServ->NotifyObservers(nsnull, "application-foreground", nsnull);
        }
        break;
    }

    case AndroidGeckoEvent::SCREENORIENTATION_CHANGED: {
        hal::NotifyScreenOrientationChange(static_cast<dom::ScreenOrientation>(curEvent->ScreenOrientation()));
        break;
    }

    default:
        nsWindow::OnGlobalAndroidEvent(curEvent);
    }

    EVLOG("nsAppShell: -- done event %p %d", (void*)curEvent.get(), curEvent->Type());

    return true;
}

void
nsAppShell::ResendLastResizeEvent(nsWindow* aDest) {
    if (gLastSizeChange) {
        nsWindow::OnGlobalAndroidEvent(gLastSizeChange);
    }
}

AndroidGeckoEvent*
nsAppShell::PopNextEvent()
{
    AndroidGeckoEvent *ae = nsnull;
    MutexAutoLock lock(mQueueLock);
    if (mEventQueue.Length()) {
        ae = mEventQueue[0];
        mEventQueue.RemoveElementAt(0);
        if (mQueuedDrawEvent == ae) {
            mQueuedDrawEvent = nsnull;
        } else if (mQueuedViewportEvent == ae) {
            mQueuedViewportEvent = nsnull;
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
        
        
        
        bool allowCoalescingNextViewport = false;

        MutexAutoLock lock(mQueueLock);
        EVLOG("nsAppShell::PostEvent %p %d", ae, ae->Type());
        switch (ae->Type()) {
        case AndroidGeckoEvent::SURFACE_DESTROYED:
            
            
            mEventQueue.InsertElementAt(0, ae);
            AndroidGeckoEvent *event;
            for (int i = mEventQueue.Length() - 1; i >= 1; i--) {
                event = mEventQueue[i];
                if (event->Type() == AndroidGeckoEvent::SURFACE_CREATED) {
                    EVLOG("nsAppShell: Dropping old SURFACE_CREATED event at %p %d", event, i);
                    mEventQueue.RemoveElementAt(i);
                    delete event;
                }
            }
            break;

        case AndroidGeckoEvent::COMPOSITOR_PAUSE:
        case AndroidGeckoEvent::COMPOSITOR_RESUME:
            
            {
                int i = 0;
                while (i < mEventQueue.Length() &&
                       (mEventQueue[i]->Type() == AndroidGeckoEvent::COMPOSITOR_PAUSE ||
                        mEventQueue[i]->Type() == AndroidGeckoEvent::COMPOSITOR_RESUME)) {
                    i++;
                }
                EVLOG("nsAppShell: Inserting compositor event %d at position %d to maintain priority order", ae->Type(), i);
                mEventQueue.InsertElementAt(i, ae);
            }
            break;

        case AndroidGeckoEvent::DRAW:
            if (mQueuedDrawEvent) {
                
                const nsIntRect& oldRect = mQueuedDrawEvent->Rect();
                const nsIntRect& newRect = ae->Rect();
                int combinedArea = (oldRect.width * oldRect.height) +
                                   (newRect.width * newRect.height);

                nsIntRect combinedRect = oldRect.Union(newRect);
                
                
                
                int boundsArea = combinedRect.width * combinedRect.height;
                if (boundsArea > combinedArea * 8)
                    ALOG("nsAppShell: Area of bounds greatly exceeds combined area: %d > %d",
                         boundsArea, combinedArea);

                
                
                
                ae->Init(AndroidGeckoEvent::DRAW, combinedRect);

                EVLOG("nsAppShell: Coalescing previous DRAW event at %p into new DRAW event %p", mQueuedDrawEvent, ae);
                mEventQueue.RemoveElement(mQueuedDrawEvent);
                delete mQueuedDrawEvent;
            }

            if (!mAllowCoalescingNextDraw) {
                
                
                
                mAllowCoalescingNextDraw = true;
                mQueuedDrawEvent = nsnull;
            } else {
                mQueuedDrawEvent = ae;
            }

            allowCoalescingNextViewport = true;

            mEventQueue.AppendElement(ae);
            break;

        case AndroidGeckoEvent::VIEWPORT:
            if (mQueuedViewportEvent) {
                
                EVLOG("nsAppShell: Dropping old viewport event at %p in favour of new VIEWPORT event %p", mQueuedViewportEvent, ae);
                mEventQueue.RemoveElement(mQueuedViewportEvent);
                delete mQueuedViewportEvent;
            }
            mQueuedViewportEvent = ae;
            
            
            mAllowCoalescingNextDraw = false;
            allowCoalescingNextViewport = true;

            mEventQueue.AppendElement(ae);
            break;

        case AndroidGeckoEvent::MOTION_EVENT:
            if (ae->Action() == AndroidMotionEvent::ACTION_MOVE) {
                int len = mEventQueue.Length();
                if (len > 0) {
                    AndroidGeckoEvent* event = mEventQueue[len - 1];
                    if (event->Type() == AndroidGeckoEvent::MOTION_EVENT && event->Action() == AndroidMotionEvent::ACTION_MOVE) {
                        
                        EVLOG("nsAppShell: Dropping old move event at %p in favour of new move event %p", event, ae);
                        mEventQueue.RemoveElementAt(len - 1);
                        delete event;
                    }
                }
            }
            mEventQueue.AppendElement(ae);
            break;

        case AndroidGeckoEvent::NATIVE_POKE:
            allowCoalescingNextViewport = true;
            

        default:
            mEventQueue.AppendElement(ae);
            break;
        }

        
        
        
        if (!allowCoalescingNextViewport)
            mQueuedViewportEvent = nsnull;
    }
    NotifyNativeEvent();
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
    return nsAppShell::gAppShell->ProcessNextNativeEvent(true) ? true : false;
}

void NotifyEvent()
{
    nsAppShell::gAppShell->NotifyNativeEvent();
}

}
