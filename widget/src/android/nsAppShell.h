





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsInterfaceHashtable.h"

#include "prcvar.h"

namespace mozilla {
class AndroidGeckoEvent;
bool ProcessNextEvent();
void NotifyEvent();
}

class nsAppShell :
    public nsBaseAppShell
{
public:
    static nsAppShell *gAppShell;
    static mozilla::AndroidGeckoEvent *gEarlyEvent;

    nsAppShell();

    nsresult Init();

    void NotifyNativeEvent();

    virtual PRBool ProcessNextNativeEvent(PRBool mayWait);

    void PostEvent(mozilla::AndroidGeckoEvent *event);
    void RemoveNextEvent();
    void OnResume();

    nsresult AddObserver(const nsAString &aObserverKey, nsIObserver *aObserver);
    void CallObserver(const nsAString &aObserverKey, const nsAString &aTopic, const nsAString &aData);
    void RemoveObserver(const nsAString &aObserverKey);

protected:
    virtual void ScheduleNativeEventCallback();
    virtual ~nsAppShell();

    int mNumDraws;
    PRLock *mQueueLock;
    PRLock *mCondLock;
    PRCondVar *mQueueCond;
    nsTArray<mozilla::AndroidGeckoEvent *> mEventQueue;
    nsInterfaceHashtable<nsStringHashKey, nsIObserver> mObserversHash;

    mozilla::AndroidGeckoEvent *GetNextEvent();
    mozilla::AndroidGeckoEvent *PeekNextEvent();
};

#endif 

