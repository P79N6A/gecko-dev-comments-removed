





































#ifndef mozilla_dom_ContentProcessParent_h
#define mozilla_dom_ContentProcessParent_h

#include "base/waitable_event_watcher.h"

#include "mozilla/dom/PContentProcessParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "nsIObserver.h"
#include "mozilla/Monitor.h"

namespace mozilla {

namespace ipc {
class TestShellParent;
}

namespace dom {

class TabParent;

class ContentProcessParent : private PContentProcessParent,
                             public base::WaitableEventWatcher::Delegate,
                             public nsIObserver
{
private:
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;

public:
    static ContentProcessParent* GetSingleton();

#if 0
    
    static ContentProcessParent* FreeSingleton();
#endif

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    virtual void OnWaitableEventSignaled(base::WaitableEvent *event);

    TabParent* CreateTab();
    mozilla::ipc::TestShellParent* CreateTestShell();

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
};

} 
} 

#endif
