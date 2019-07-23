





































#ifndef mozilla_dom_ContentProcessChild_h
#define mozilla_dom_ContentProcessChild_h

#include "mozilla/dom/PContentProcessProtocolChild.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

class ContentProcessChild
    : public PContentProcessProtocolChild
{
public:
    ContentProcessChild();
    virtual ~ContentProcessChild();

    bool Init(MessageLoop* aIOLoop, IPC::Channel* aChannel);

    static ContentProcessChild* GetSingleton() {
        NS_ASSERTION(sSingleton, "not initialized");
        return sSingleton;
    }

    virtual PIFrameEmbeddingProtocolChild* PIFrameEmbeddingConstructor(const MagicWindowHandle& hwnd);
    virtual nsresult PIFrameEmbeddingDestructor(PIFrameEmbeddingProtocolChild*);

    virtual PTestShellProtocolChild* PTestShellConstructor();
    virtual nsresult PTestShellDestructor(PTestShellProtocolChild*);

    void Quit();
    virtual nsresult RecvQuit();

private:
    static ContentProcessChild* sSingleton;

    nsTArray<nsAutoPtr<PIFrameEmbeddingProtocolChild> > mIFrames;
    nsTArray<nsAutoPtr<PTestShellProtocolChild> > mTestShells;

    PRBool mQuit;

    DISALLOW_EVIL_CONSTRUCTORS(ContentProcessChild);
};

} 
} 

#endif
