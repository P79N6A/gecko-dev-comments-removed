








































#include "nsIdleService.h"
#include "nsString.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "nsCOMArray.h"


#define OBSERVER_TOPIC_IDLE "idle"
#define OBSERVER_TOPIC_BACK "back"
#define OBSERVER_TOPIC_IDLE_DAILY "idle-daily"

#define MIN_IDLE_POLL_INTERVAL 5000
#define MAX_IDLE_POLL_INTERVAL 300000

#define PREF_LAST_DAILY "idle.lastDailyNotification"


class IdleListenerComparator
{
public:
    PRBool Equals(IdleListener a, IdleListener b) const
    {
        return (a.observer == b.observer) &&
               (a.reqIdleTime == b.reqIdleTime);
    }
};

nsIdleService::nsIdleService()
{
    
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
    StartTimer(MAX_IDLE_POLL_INTERVAL);
}

nsIdleService::~nsIdleService()
{
    StopTimer();
}

NS_IMETHODIMP
nsIdleService::AddIdleObserver(nsIObserver* aObserver, PRUint32 aIdleTime)
{
    NS_ENSURE_ARG_POINTER(aObserver);
    NS_ENSURE_ARG(aIdleTime);

    
    IdleListener listener(aObserver, aIdleTime * 1000);

    if (!mArrayListeners.AppendElement(listener))
        return NS_ERROR_OUT_OF_MEMORY;

    
    if (!mTimer) {
        nsresult rv;
        mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    CheckAwayState();

    return NS_OK;
}

NS_IMETHODIMP
nsIdleService::RemoveIdleObserver(nsIObserver* aObserver, PRUint32 aTime)
{
    NS_ENSURE_ARG_POINTER(aObserver);
    NS_ENSURE_ARG(aTime);
    IdleListener listener(aObserver, aTime * 1000);

    
    IdleListenerComparator c;
    if (mArrayListeners.RemoveElement(listener, c)) {
        if (mArrayListeners.IsEmpty()) {
            StopTimer();
        }
        return NS_OK;
    }

    
    return NS_ERROR_FAILURE;
}

void
nsIdleService::IdleTimerCallback(nsITimer* aTimer, void* aClosure)
{
    static_cast<nsIdleService*>(aClosure)->CheckAwayState();
}

void
nsIdleService::CheckAwayState()
{
    
    PRUint32 idleTime;
    if (NS_FAILED(GetIdleTime(&idleTime)))
        return;

    
    PRUint32 nextPoll = MAX_IDLE_POLL_INTERVAL;

    nsAutoString timeStr;
    timeStr.AppendInt(idleTime);

    
    
    nsCOMArray<nsIObserver> idleListeners;
    nsCOMArray<nsIObserver> hereListeners;
    for (PRUint32 i = 0; i < mArrayListeners.Length(); i++) {
        IdleListener& curListener = mArrayListeners.ElementAt(i);

        
        PRUint32 curPoll = curListener.reqIdleTime - idleTime;

        
        if (!curListener.isIdle) {
            
            if (idleTime >= curListener.reqIdleTime) {
                curListener.isIdle = PR_TRUE;
                idleListeners.AppendObject(curListener.observer);

                
                curPoll = MIN_IDLE_POLL_INTERVAL;
            }
        }
        
        else {
            
            if (idleTime < curListener.reqIdleTime) {
                curListener.isIdle = PR_FALSE;
                hereListeners.AppendObject(curListener.observer);
            }
            
            else {
                curPoll = MIN_IDLE_POLL_INTERVAL;
            }
        }

        
        nextPoll = PR_MIN(nextPoll, curPoll);
    }

    
    for (PRInt32 i = 0; i < idleListeners.Count(); i++) {
        idleListeners[i]->Observe(this, OBSERVER_TOPIC_IDLE, timeStr.get());
    }

    
    for (PRInt32 i = 0; i < hereListeners.Count(); i++) {
        hereListeners[i]->Observe(this, OBSERVER_TOPIC_BACK, timeStr.get());
    }

    
    if (idleTime >= MAX_IDLE_POLL_INTERVAL) {
        nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (pref) {
            
            PRUint32 nowSec = PR_Now() / PR_USEC_PER_SEC;

            
            PRInt32 lastDaily = 0;
            pref->GetIntPref(PREF_LAST_DAILY, &lastDaily);

            
            if (nowSec - lastDaily > 86400) {
                nsCOMPtr<nsIObserverService> observerService =
                    do_GetService("@mozilla.org/observer-service;1");
                observerService->NotifyObservers(nsnull,
                                                 OBSERVER_TOPIC_IDLE_DAILY,
                                                 nsnull);

                pref->SetIntPref(PREF_LAST_DAILY, nowSec);
            }
        }
    }

    
    StartTimer(nextPoll);
}

void
nsIdleService::StartTimer(PRUint32 aDelay)
{
    if (mTimer) {
        StopTimer();
        mTimer->InitWithFuncCallback(IdleTimerCallback, this, aDelay,
                                     nsITimer::TYPE_ONE_SHOT);
    }
}

void
nsIdleService::StopTimer()
{
    if (mTimer) {
        mTimer->Cancel();
    }
}

void
nsIdleService::IdleTimeWasModified()
{
    StartTimer(0);
}
