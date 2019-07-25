





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

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

protected:
    virtual void ScheduleNativeEventCallback();
    virtual ~nsAppShell();

    int mNumDraws;
    PRLock *mQueueLock;
    PRLock *mCondLock;
    PRLock *mPausedLock;
    PRCondVar *mQueueCond;
    PRCondVar *mPaused;
    nsTArray<mozilla::AndroidGeckoEvent *> mEventQueue;

    mozilla::AndroidGeckoEvent *GetNextEvent();
    mozilla::AndroidGeckoEvent *PeekNextEvent();
};

#endif 

