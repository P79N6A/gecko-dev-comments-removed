


#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "mozilla/dom/IFrameEmbeddingProtocolParent.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

class nsIURI;

namespace mozilla {
namespace dom {

class TabParent
    : public IFrameEmbeddingProtocolParent
{
public:
    TabParent();
    virtual ~TabParent();

    void LoadURL(nsIURI* aURI);
    void Move(PRUint32 x, PRUint32 y, PRUint32 width, PRUint32 height);
};

} 
} 

#endif
