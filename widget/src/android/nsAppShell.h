





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "nsBaseAppShell.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsInterfaceHashtable.h"

namespace mozilla {
class AndroidGeckoEvent;
bool ProcessNextEvent();
void NotifyEvent();
}

class nsWindow;

class nsAppShell :
    public nsBaseAppShell
{
    typedef mozilla::CondVar CondVar;
    typedef mozilla::Mutex Mutex;

public:
    static nsAppShell *gAppShell;
    static mozilla::AndroidGeckoEvent *gEarlyEvent;

    nsAppShell();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIOBSERVER

    nsresult Init();

    void NotifyNativeEvent();

    virtual bool ProcessNextNativeEvent(bool mayWait);

    void PostEvent(mozilla::AndroidGeckoEvent *event);
    void RemoveNextEvent();
    void OnResume();

    nsresult AddObserver(const nsAString &aObserverKey, nsIObserver *aObserver);
    void CallObserver(const nsAString &aObserverKey, const nsAString &aTopic, const nsAString &aData);
    void RemoveObserver(const nsAString &aObserverKey);
    void NotifyObservers(nsISupports *aSupports, const char *aTopic, const PRUnichar *aData);
    void ResendLastResizeEvent(nsWindow* aDest);

protected:
    virtual void ScheduleNativeEventCallback();
    virtual ~nsAppShell();

    Mutex mQueueLock;
    Mutex mCondLock;
    CondVar mQueueCond;
    int mNumDraws;
    nsTArray<mozilla::AndroidGeckoEvent *> mEventQueue;
    nsInterfaceHashtable<nsStringHashKey, nsIObserver> mObserversHash;

    mozilla::AndroidGeckoEvent *GetNextEvent();
    mozilla::AndroidGeckoEvent *PeekNextEvent();
};

#endif 

