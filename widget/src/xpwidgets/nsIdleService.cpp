







































#include "nsIdleService.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "nsCOMArray.h"


#define OBSERVER_TOPIC_IDLE "idle"
#define OBSERVER_TOPIC_BACK "back"

#define IDLE_POLL_INTERVAL 5000




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
}

nsIdleService::~nsIdleService()
{
    if (mTimer)
        mTimer->Cancel();
}

NS_IMETHODIMP
nsIdleService::AddIdleObserver(nsIObserver* aObserver, PRUint32 aIdleTime)
{
    NS_ENSURE_ARG_POINTER(aObserver);
    NS_ENSURE_ARG(aIdleTime);

    
    IdleListener listener(aObserver, aIdleTime);

    if (!mArrayListeners.AppendElement(listener))
        return NS_ERROR_OUT_OF_MEMORY;

    
    if (!mTimer)
    {
        nsresult rv;
        mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
        if (NS_FAILED(rv))
            return rv;
        mTimer->InitWithFuncCallback(IdleTimerCallback, this, IDLE_POLL_INTERVAL,
                                     nsITimer::TYPE_REPEATING_SLACK);
    }

    
    CheckAwayState();

    return NS_OK;
}

NS_IMETHODIMP
nsIdleService::RemoveIdleObserver(nsIObserver* aObserver, PRUint32 aTime)
{
    NS_ENSURE_ARG_POINTER(aObserver);
    NS_ENSURE_ARG(aTime);
    IdleListener listener(aObserver, aTime);

    
    IdleListenerComparator c;
    if (mArrayListeners.RemoveElement(listener, c))
    {
        if (mTimer && mArrayListeners.IsEmpty())
        {
            mTimer->Cancel();
            mTimer = nsnull;
        }
        return NS_OK;
    }

    
    return NS_ERROR_FAILURE;
}

void
nsIdleService::IdleTimerCallback(nsITimer* aTimer, void* aClosure)
{
    NS_STATIC_CAST(nsIdleService*, aClosure)->CheckAwayState();
}

void
nsIdleService::CheckAwayState()
{
    
    PRUint32 idleTime;
    if (NS_FAILED(GetIdleTime(&idleTime)))
        return;

    nsAutoString timeStr;
    timeStr.AppendInt(idleTime);

    
    
    nsCOMArray<nsIObserver> idleListeners;
    nsCOMArray<nsIObserver> hereListeners;
    for (PRUint32 i = 0; i < mArrayListeners.Length(); i++)
    {
        IdleListener& curListener = mArrayListeners.ElementAt(i);
        if ((curListener.reqIdleTime * 1000 <= idleTime) &&
            !curListener.isIdle)
        {
            curListener.isIdle = PR_TRUE;
            idleListeners.AppendObject(curListener.observer);
        }
        else if ((curListener.reqIdleTime * 1000 > idleTime) &&
                 curListener.isIdle)
        {
            curListener.isIdle = PR_FALSE;
            hereListeners.AppendObject(curListener.observer);
        }
    }

    
    for (PRInt32 i = 0; i < idleListeners.Count(); i++)
    {
        idleListeners[i]->Observe(this, OBSERVER_TOPIC_IDLE, timeStr.get());
    }

    
    for (PRInt32 i = 0; i < hereListeners.Count(); i++)
    {
        hereListeners[i]->Observe(this, OBSERVER_TOPIC_BACK, timeStr.get());
    }
}

