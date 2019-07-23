





































#include "ContentProcessChild.h"
#include "TabChild.h"

#include "mozilla/ipc/TestShellChild.h"
#include "mozilla/net/NeckoChild.h"

#include "nsXULAppAPI.h"

#include "base/message_loop.h"
#include "base/task.h"

using namespace mozilla::ipc;
using namespace mozilla::net;

namespace mozilla {
namespace dom {

ContentProcessChild* ContentProcessChild::sSingleton;

ContentProcessChild::ContentProcessChild()
    : mQuit(PR_FALSE)
{
}

ContentProcessChild::~ContentProcessChild()
{
}

bool
ContentProcessChild::Init(MessageLoop* aIOLoop, IPC::Channel* aChannel)
{
    NS_ASSERTION(!sSingleton, "only one ContentProcessChild per child");
  
    Open(aChannel, aIOLoop);
    sSingleton = this;

    return true;
}

PIFrameEmbeddingChild*
ContentProcessChild::PIFrameEmbeddingConstructor(const MagicWindowHandle& hwnd)
{
    PIFrameEmbeddingChild* iframe = new TabChild(hwnd);
    if (iframe && mIFrames.AppendElement(iframe)) {
        return iframe;
    }
    delete iframe;
    return nsnull;
}

nsresult
ContentProcessChild::PIFrameEmbeddingDestructor(PIFrameEmbeddingChild* iframe)
{
    mIFrames.RemoveElement(iframe);
    return NS_OK;
}

PTestShellChild*
ContentProcessChild::PTestShellConstructor()
{
    PTestShellChild* testshell = new TestShellChild();
    if (testshell && mTestShells.AppendElement(testshell)) {
        return testshell;
    }
    delete testshell;
    return nsnull;
}

nsresult
ContentProcessChild::PTestShellDestructor(PTestShellChild* shell)
{
    mTestShells.RemoveElement(shell);
    return NS_OK;
}

PNeckoChild* 
ContentProcessChild::PNeckoConstructor()
{
    return new NeckoChild();
}

nsresult 
ContentProcessChild::PNeckoDestructor(PNeckoChild* necko)
{
    delete necko;
    return NS_OK;
}

void
ContentProcessChild::Quit()
{
    NS_ASSERTION(mQuit, "Exiting uncleanly!");
    mIFrames.Clear();
    mTestShells.Clear();
}

static void
QuitIOLoop()
{
    MessageLoop::current()->Quit();
}

nsresult
ContentProcessChild::RecvQuit()
{
    mQuit = PR_TRUE;

    Quit();

    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     NewRunnableFunction(&QuitIOLoop));

    return NS_OK;
}

} 
} 
