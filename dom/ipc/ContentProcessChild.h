





































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

    bool Init(MessageLoop* aIOLoop, IPC::Channel* aChannel);

    static ContentProcessChild* GetSingleton() {
        NS_ASSERTION(sSingleton, "not initialized");
        return sSingleton;
    }

    virtual PIFrameEmbeddingChild* PIFrameEmbeddingConstructor(
            const MagicWindowHandle& hwnd);
    virtual bool PIFrameEmbeddingDestructor(PIFrameEmbeddingChild*);

    virtual PTestShellChild* PTestShellConstructor();
    virtual bool PTestShellDestructor(PTestShellChild*);

    virtual PNeckoChild* PNeckoConstructor();
    virtual bool PNeckoDestructor(PNeckoChild*);

    void Quit();
    virtual bool RecvQuit();

private:
    static ContentProcessChild* sSingleton;

    nsTArray<nsAutoPtr<PIFrameEmbeddingChild> > mIFrames;
    nsTArray<nsAutoPtr<PTestShellChild> > mTestShells;

    PRBool mQuit;

    DISALLOW_EVIL_CONSTRUCTORS(ContentProcessChild);
};

} 
} 

#endif
