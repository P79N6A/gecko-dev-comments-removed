


#ifndef mozilla_dom_ContentProcessParent_h
#define mozilla_dom_ContentProcessParent_h

#include "mozilla/dom/ContentProcessProtocolParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

namespace mozilla {
namespace dom {

class TabParent;

class ContentProcessParent
    : private ContentProcessProtocolParent
{
private:
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;

public:
    static ContentProcessParent* GetSingleton();

#if 0
    
    static ContentProcessParent* FreeSingleton();
#endif

    TabParent* CreateTab(const MagicWindowHandle& hwnd);

private:
    static ContentProcessParent* gSingleton;

    
    
    using ContentProcessProtocolParent::SendIFrameEmbeddingConstructor;

    ContentProcessParent();
    virtual ~ContentProcessParent();

    virtual IFrameEmbeddingProtocolParent* IFrameEmbeddingConstructor(const MagicWindowHandle& parentWidget);
    virtual nsresult IFrameEmbeddingDestructor(IFrameEmbeddingProtocolParent* frame);

    GeckoChildProcessHost mSubprocess;
};

} 
} 

#endif
