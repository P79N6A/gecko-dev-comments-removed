


#ifndef mozilla_dom_ContentProcessParent_h
#define mozilla_dom_ContentProcessParent_h

#include "base/waitable_event_watcher.h"

#include "mozilla/dom/ContentProcessProtocolParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "nsIObserver.h"
#include "mozilla/Monitor.h"

namespace mozilla {
namespace ipc {
class TestShellParent;
}

namespace dom {

class TabParent;

class ContentProcessParent
    : private ContentProcessProtocolParent,
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

    TabParent* CreateTab(const MagicWindowHandle& hwnd);
    mozilla::ipc::TestShellParent* CreateTestShell();

private:
    static ContentProcessParent* gSingleton;

    
    
    using ContentProcessProtocolParent::SendIFrameEmbeddingConstructor;
    using ContentProcessProtocolParent::SendTestShellConstructor;

    ContentProcessParent();
    virtual ~ContentProcessParent();

    virtual IFrameEmbeddingProtocolParent* IFrameEmbeddingConstructor(const MagicWindowHandle& parentWidget);
    virtual nsresult IFrameEmbeddingDestructor(IFrameEmbeddingProtocolParent* frame);

    virtual TestShellProtocolParent* TestShellConstructor();
    virtual nsresult TestShellDestructor(TestShellProtocolParent* shell);

    mozilla::Monitor mMonitor;

    GeckoChildProcessHost* mSubprocess;
};

} 
} 

#endif
