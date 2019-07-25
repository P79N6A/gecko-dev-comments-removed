





































#ifndef mozilla_dom_ContentProcessParent_h
#define mozilla_dom_ContentProcessParent_h

#include "base/waitable_event_watcher.h"

#include "mozilla/dom/PContentProcessParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "mozilla/Monitor.h"

namespace mozilla {

namespace ipc {
class TestShellParent;
}

namespace dom {

class TabParent;

class ContentProcessParent : public PContentProcessParent
                           , public nsIObserver
                           , public nsIThreadObserver
{
private:
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;
    typedef mozilla::ipc::TestShellParent TestShellParent;

public:
    static ContentProcessParent* GetSingleton();

#if 0
    
    static ContentProcessParent* FreeSingleton();
#endif

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSITHREADOBSERVER

    TabParent* CreateTab();

    TestShellParent* CreateTestShell();
    bool DestroyTestShell(TestShellParent* aTestShell);

    bool RequestRunToCompletion();

    bool IsAlive();

protected:
    virtual void ActorDestroy(ActorDestroyReason why);

private:
    static ContentProcessParent* gSingleton;

    
    
    using PContentProcessParent::SendPIFrameEmbeddingConstructor;
    using PContentProcessParent::SendPTestShellConstructor;

    ContentProcessParent();
    virtual ~ContentProcessParent();

    virtual PIFrameEmbeddingParent* AllocPIFrameEmbedding();
    virtual bool DeallocPIFrameEmbedding(PIFrameEmbeddingParent* frame);

    virtual PTestShellParent* AllocPTestShell();
    virtual bool DeallocPTestShell(PTestShellParent* shell);

    virtual PNeckoParent* AllocPNecko();
    virtual bool DeallocPNecko(PNeckoParent* necko);

    mozilla::Monitor mMonitor;

    GeckoChildProcessHost* mSubprocess;

    int mRunToCompletionDepth;
    nsCOMPtr<nsIThreadObserver> mOldObserver;

    bool mIsAlive;
};

} 
} 

#endif
