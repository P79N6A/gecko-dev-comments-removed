





































#ifndef mozilla_dom_ContentProcessChild_h
#define mozilla_dom_ContentProcessChild_h

#include "mozilla/dom/PContentProcessChild.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

class ContentProcessChild : public PContentProcessChild
{
public:
    ContentProcessChild();
    virtual ~ContentProcessChild();

    bool Init(MessageLoop* aIOLoop,
              base::ProcessHandle aParentHandle,
              IPC::Channel* aChannel);

    static ContentProcessChild* GetSingleton() {
        NS_ASSERTION(sSingleton, "not initialized");
        return sSingleton;
    }

    virtual PIFrameEmbeddingChild* AllocPIFrameEmbedding();
    virtual bool DeallocPIFrameEmbedding(PIFrameEmbeddingChild*);

    virtual PTestShellChild* AllocPTestShell();
    virtual bool DeallocPTestShell(PTestShellChild*);

    virtual PNeckoChild* AllocPNecko();
    virtual bool DeallocPNecko(PNeckoChild*);

private:
    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why);

    void Quit();

    static ContentProcessChild* sSingleton;

    nsTArray<PIFrameEmbeddingChild* > mIFrames;
    nsTArray<nsAutoPtr<PTestShellChild> > mTestShells;

    PRBool mQuit;

    DISALLOW_EVIL_CONSTRUCTORS(ContentProcessChild);
};

} 
} 

#endif
