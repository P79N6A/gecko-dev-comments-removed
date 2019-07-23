





































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
ContentProcessChild::Init(MessageLoop* aIOLoop,
                          base::ProcessHandle aParentHandle,
                          IPC::Channel* aChannel)
{
    NS_ASSERTION(!sSingleton, "only one ContentProcessChild per child");
  
    Open(aChannel, aParentHandle, aIOLoop);
    sSingleton = this;

    return true;
}

PIFrameEmbeddingChild*
ContentProcessChild::AllocPIFrameEmbedding()
{
    PIFrameEmbeddingChild* iframe = new TabChild();
    if (iframe && mIFrames.AppendElement(iframe)) {
        return iframe;
    }
    delete iframe;
    return nsnull;
}

bool
ContentProcessChild::DeallocPIFrameEmbedding(PIFrameEmbeddingChild* iframe)
{
    mIFrames.RemoveElement(iframe);
    return true;
}

PTestShellChild*
ContentProcessChild::AllocPTestShell()
{
    PTestShellChild* testshell = new TestShellChild();
    if (testshell && mTestShells.AppendElement(testshell)) {
        return testshell;
    }
    delete testshell;
    return nsnull;
}

bool
ContentProcessChild::DeallocPTestShell(PTestShellChild* shell)
{
    mTestShells.RemoveElement(shell);
    return true;
}

PNeckoChild* 
ContentProcessChild::AllocPNecko()
{
    return new NeckoChild();
}

bool 
ContentProcessChild::DeallocPNecko(PNeckoChild* necko)
{
    delete necko;
    return true;
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

bool
ContentProcessChild::RecvQuit()
{
    mQuit = PR_TRUE;

    Quit();

    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     NewRunnableFunction(&QuitIOLoop));

    return true;
}

} 
} 
