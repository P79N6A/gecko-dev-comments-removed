





































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

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIOBSERVER

    nsresult Init();

    void NotifyNativeEvent();

    virtual PRBool ProcessNextNativeEvent(PRBool mayWait);

    void PostEvent(mozilla::AndroidGeckoEvent *event);
    void RemoveNextEvent();
    void OnResume();

    nsresult AddObserver(const nsAString &aObserverKey, nsIObserver *aObserver);
    void CallObserver(const nsAString &aObserverKey, const nsAString &aTopic, const nsAString &aData);
    void RemoveObserver(const nsAString &aObserverKey);
    void NotifyObservers(nsISupports *aSupports, const char *aTopic, const PRUnichar *aData);

protected:
    virtual void ScheduleNativeEventCallback();
    virtual ~nsAppShell();

    PRLock *mQueueLock;
    PRLock *mCondLock;
    PRCondVar *mQueueCond;
    int mNumDraws;
    nsTArray<mozilla::AndroidGeckoEvent *> mEventQueue;
    nsInterfaceHashtable<nsStringHashKey, nsIObserver> mObserversHash;

    mozilla::AndroidGeckoEvent *GetNextEvent();
    mozilla::AndroidGeckoEvent *PeekNextEvent();
};

#endif 

