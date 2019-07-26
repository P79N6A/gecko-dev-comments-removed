




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

class LoadInfoUpdateRunner;
class LoadInfoCollectRunner;

class LoadMonitor MOZ_FINAL : public nsIObserver
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIOBSERVER

    LoadMonitor();
    ~LoadMonitor();

    nsresult Init(nsRefPtr<LoadMonitor> &self);
    void Shutdown();
    float GetSystemLoad();
    float GetProcessLoad();

    friend class LoadInfoCollectRunner;

private:

    void SetProcessLoad(float load);
    void SetSystemLoad(float load);

    mozilla::Mutex       mLock;
    mozilla::CondVar     mCondVar;
    bool                 mShutdownPending;
    nsCOMPtr<nsIThread>  mLoadInfoThread;
    float                mSystemLoad;
    float                mProcessLoad;
};

#endif 
