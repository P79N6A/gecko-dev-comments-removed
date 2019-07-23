







































#ifndef nsIdleService_h__
#define nsIdleService_h__

#include "nsIIdleService.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsIObserver.h"



class IdleListener {
public:
    nsCOMPtr<nsIObserver> observer;
    PRUint32 reqIdleTime;
    PRBool isIdle;

    IdleListener(nsIObserver* obs, PRUint32 reqIT, PRBool aIsIdle = PR_FALSE) :
        observer(obs), reqIdleTime(reqIT), isIdle(aIsIdle) {}
    ~IdleListener() {}
};

class nsIdleService : public nsIIdleService
{
public:
    nsIdleService();

    
    
    NS_IMETHOD AddIdleObserver(nsIObserver* aObserver, PRUint32 aIdleTime);
    NS_IMETHOD RemoveIdleObserver(nsIObserver* aObserver, PRUint32 aIdleTime);

    static void IdleTimerCallback(nsITimer* aTimer, void* aClosure);
    
    void IdleTimeWasModified();

protected:
    void CheckAwayState();
    ~nsIdleService();

private:
    void StartTimer(PRUint32 aDelay);
    void StopTimer();
    nsCOMPtr<nsITimer> mTimer;
    nsTArray<IdleListener> mArrayListeners;
};

#endif 
