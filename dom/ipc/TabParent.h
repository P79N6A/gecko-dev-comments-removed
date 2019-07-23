


#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "TabTypes.h"
#include "IFrameEmbeddingProtocol.h"
#include "IFrameEmbeddingProtocolParent.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

class nsIURI;

namespace mozilla {
namespace tabs {

class TabParent
    : private IFrameEmbeddingProtocolParent
{
private:
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;

public:
    TabParent(MagicWindowHandle parentWidget);
    virtual ~TabParent();

    void LoadURL(nsIURI* aURI);
    void Move(PRUint32 x, PRUint32 y, PRUint32 width, PRUint32 height);

private:
    GeckoChildProcessHost mSubprocess;
};

} 
} 

#endif
