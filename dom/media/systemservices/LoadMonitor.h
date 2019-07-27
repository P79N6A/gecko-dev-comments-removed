




#ifndef _LOADMONITOR_H_
#define _LOADMONITOR_H_

#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Atomics.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "nsIObserver.h"

namespace mozilla {
class LoadInfoCollectRunner;

class LoadNotificationCallback
{
public:
    virtual void LoadChanged(float aSystemLoad, float aProcessLoad) = 0;
};

class LoadMonitor final : public nsIObserver
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit LoadMonitor(int aLoadUpdateInterval);

    nsresult Init(nsRefPtr<LoadMonitor> &self);
    void SetLoadChangeCallback(LoadNotificationCallback* aCallback);
    void Shutdown();
    float GetSystemLoad();
    float GetProcessLoad();

    friend class LoadInfoCollectRunner;

private:
    ~LoadMonitor();

    void SetProcessLoad(float load);
    void SetSystemLoad(float load);
    void FireCallbacks();

    int                  mLoadUpdateInterval;
    mozilla::Mutex       mLock;
    mozilla::CondVar     mCondVar;
    bool                 mShutdownPending;
    nsCOMPtr<nsIThread>  mLoadInfoThread;
    float                mSystemLoad;
    float                mProcessLoad;
    LoadNotificationCallback* mLoadNotificationCallback;
};

} 

#endif 
